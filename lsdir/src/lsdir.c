#define _GNU_SOURCE
#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <limits.h>
#include <string.h>
#include <fnmatch.h>
#include <stdint.h>
#include <errno.h>

#define BUF_SIZE 1024
#define handle_error(msg) \
       do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct linux_dirent {
   long           d_ino;
   off_t          d_off;
   unsigned short d_reclen;
   char           d_name[];
};

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};


char GetItemType(const char *pPath, int resolveLink, off_t *pFileSize)
{
    struct stat st = {0};
    char iType = '\0';
    if(0 == pPath)
    {
        return '\0';
    }
    if(resolveLink)
    {
        if(-1 == stat(pPath, &st))
        {
            return '\0';
        }
    }
    else
    {
        if(-1 == lstat(pPath, &st))
        {
            return '\0';
        }
    }
    *pFileSize = st.st_size;
    if( S_ISREG(st.st_mode) )
    {
        iType = 'r';
    }
    else if( S_ISDIR(st.st_mode) )
    {
        iType = 'd';
    }
    else if( S_ISFIFO(st.st_mode) )
    {
        iType = 'f';
    }
    else if( S_ISSOCK(st.st_mode) )
    {
        iType = 's';
    }
    else if( S_ISLNK(st.st_mode) )
    {
        iType = 'l';
    }
    else if( S_ISBLK(st.st_mode) )
    {
        iType = 'b';
    }
    else if( S_ISCHR(st.st_mode) )
    {
        iType = 'c';
    }
    return iType;
}

int MatchWildcards(const char *name, char *wildcards, const unsigned int wildcardsLen)
{
    if (NULL == name || NULL == wildcards)
    {
        return 0;
    }
    const char *end = wildcards + wildcardsLen;
    char *pattern = wildcards;
    while(pattern < end && '\0' != (*pattern))
    {
        if(0 == fnmatch(pattern, name, 0))
        {
            return 0;
        }
        pattern += strlen(pattern) + 1;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int fd = -1;
    int nRead = 0;
    char buffer[BUF_SIZE] = {0};
    //struct linux_dirent64 *pDir = 0;
    struct dirent *pDir = 0;
    int bPos = 0;
    char dType = '\0';
    char iType = '\0';
    char lType = '\0';
    char *mainDir = malloc((1 < argc) ? strlen(argv[1]) + 1 : 2);
    char *tmpName = malloc(PATH_MAX); 
    char listTypes[9] = "rdfslbc";
    char listLinkTypes[9] = "rdfsbc";
    
    unsigned int iStart  = 0;
    unsigned int iEnd    = (unsigned int)(-1);
    unsigned int iCurr   = 0;
    char * fWildcards = NULL;
    char * dWildcards = NULL;
    unsigned int fWildcardsLen = 0;
    unsigned int dWildcardsLen = 0;
    unsigned int bWithSize = 0;
    
    if(!mainDir || !tmpName)
    {
        handle_error("malloc");
    }
    
    if(1 < argc)
    {
        strcpy(mainDir, argv[1]);
    }
    else
    {
        strcpy(mainDir, ".");
    }
    
    if(2 < argc)
    {
        strncpy(listTypes, argv[2], sizeof(listTypes) - 1);
    }
    if(3 < argc)
    {
        strncpy(listLinkTypes, argv[3], sizeof(listLinkTypes) - 1);
    }
    if(4 < argc)
    {
        sscanf(argv[4], "%u", &iStart);
    }
    if(5 < argc)
    {
        sscanf(argv[5], "%u", &iEnd);
    }
    if(6 < argc)
    {
        fWildcardsLen = strlen(argv[6]);
        fWildcards = malloc(fWildcardsLen + 3);
        memset(fWildcards, '\0', dWildcardsLen + 3);
        strcpy(fWildcards, argv[6]);
        printf("%s\n", argv[6]);
        char *p = strtok(fWildcards, "|");
        while(p = strtok(NULL, "|"));
    }
    if(7 < argc)
    {
        dWildcardsLen = strlen(argv[7]);
        dWildcards = malloc(dWildcardsLen + 3);
        memset(dWildcards, '\0', dWildcardsLen + 3);
        strcpy(dWildcards , argv[7]);
        char *p = strtok(dWildcards, "|");
        while(p = strtok(NULL, "|"));
    }
    if(8 < argc)
    {
        sscanf(argv[8], "%u", &bWithSize);
    }
    
    /*
    fd = open(mainDir, O_RDONLY | O_DIRECTORY);
    if(-1 == fd)
    {
       handle_error("open");
    }
    */
    DIR *dirp = opendir(mainDir);
    if(0 == dirp)
    {
       handle_error("opendir");
    }

    while(iCurr < iEnd)
    {
        pDir = readdir(dirp);
        if(0 == pDir)
        {
           break;
        }
        
        /*
        nRead = syscall(SYS_getdents64, fd, buffer, BUF_SIZE);
        //nRead = getdents64(fd, buffer, BUF_SIZE);
        if(-1 == nRead)
        {
           handle_error("getdents");
        }
        if (0 == nRead)
        {
           break;

        }

        for(bPos = 0; bPos < nRead;)
        */
        {
            /*
            pDir = (struct linux_dirent64 *) (buffer + bPos);
            // printf("%8ld  ", pDir->d_ino); 
            dType = *(buffer + bPos + pDir->d_reclen - 1);
            */
            dType = pDir->d_type;
            switch(dType)
            {
                case DT_REG:
                    iType = 'r';
                break;
                case DT_DIR:
                    iType = 'd';
                break;
                case DT_FIFO:
                    iType = 'f';
                break;
                case DT_SOCK:
                    iType = 's';
                break;
                case DT_LNK:
                    iType = 'l';
                break;
                case DT_BLK:
                    iType = 'b';
                break;
                case DT_CHR:
                    iType = 'c';
                break;
                default:
                    iType = '\0';
                break;
            }
            
            off_t totalFileSizeInBytes = -1;
            snprintf(tmpName, PATH_MAX, "%s/%s", mainDir, pDir->d_name);
            if('\0' == iType || ('l' != iType && bWithSize))
            {
                iType = GetItemType(tmpName, 0, &totalFileSizeInBytes);
            }
            
            if('l' == iType )
            {
                lType = GetItemType(tmpName, 1, &totalFileSizeInBytes);
            }

            if('\0' != iType && 0 != strchr(listTypes, iType) )
            {
                if('l' == iType)
                {
                    if('\0' != lType && 0 != strchr(listLinkTypes, lType)  && 
                       0 == MatchWildcards(pDir->d_name, ('d' == lType) ? dWildcards : fWildcards, ('d' == lType) ? dWildcardsLen : fWildcardsLen))
                    {
                        if(iCurr >= iStart)
                        {
                            fprintf(stderr, "%s//%c//1//", pDir->d_name, lType);
                            if(bWithSize)
                            {
                                fprintf(stderr, "%lld//", (long long) totalFileSizeInBytes);
                            }
                            fprintf(stderr, "\n");
                        }
                        ++iCurr;
                    }
                }
                else if(0 == MatchWildcards(pDir->d_name, ('d' == iType) ? dWildcards : fWildcards, ('d' == iType) ? dWildcardsLen : fWildcardsLen))
                {
                    if(iCurr >= iStart)
                    {
                        fprintf(stderr, "%s//%c//0//", pDir->d_name, iType);
                        if(bWithSize)
                        {
                            fprintf(stderr, "%lld//", (long long) totalFileSizeInBytes);
                        }
                        fprintf(stderr, "\n");
                    }
                    ++iCurr;
                }
            }
            bPos += pDir->d_reclen;
            
            if(iCurr >= iEnd)
            {
                break;
            }
        }
    }
    //close(fd);
    closedir(dirp);
    if(NULL != fWildcards)
    {
        free(fWildcards);
    }
    if(NULL != dWildcards)
    {
        free(dWildcards);
    }
    free(mainDir);
    free(tmpName);
    exit(EXIT_SUCCESS);
}