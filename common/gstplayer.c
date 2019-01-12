/*
 * eplayer3: command line playback using libeplayer3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gst-backend.h"

static int g_pfd[2] = {-1, -1}; /* Used to wake up select when new message in the gst bus is available */


//#include <signal.h>

int g_terminated = 0;

static void SigHandler (int signum)
{
   // Ignore it so that another Ctrl-C doesn't appear any soon
   signal(signum, SIG_IGN); 
   
   int ret = backend_stop();
   fprintf(stderr, "{\"PLAYBACK_STOP\":{\"sts\":%d, \"ctrl-c\":1}}\n", ret);
   g_terminated = 1;
   exit(-1);
}

static int WaitForCommand()
{
    struct timeval tv;
    int nfds = 1;
    
    fd_set read_fd;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;

    FD_ZERO(&read_fd);
    FD_SET(0, &read_fd);
    FD_SET(g_pfd[0], &read_fd);

    /* Get max file descriptor number for selecy */
    if(nfds < g_pfd[0] + 1)
    {
        nfds = g_pfd[0] + 1;
    }

    if(-1 == select(nfds, &read_fd, NULL, NULL, &tv))
    {
        /* Probably some interrupt occur */
        return 0;
    }

    if(FD_ISSET(g_pfd[0], &read_fd))
    {
        /* The new message in the gst bus is available */
        char tmp = 0;
        while(-1 < read(g_pfd[0], &tmp, 1) ); /* Flush pipe */
        return 2;
    }
    else if(FD_ISSET(0, &read_fd))
    {
        return 1;
    }

    /* This should not happen */
    return 0;
}

static void InitInOut()
{
    static char buff_stderr[2048];
    static char buff_stdout[2048];
    memset( buff_stderr, '\0', sizeof(buff_stderr));
    memset( buff_stdout, '\0', sizeof(buff_stdout));
    
    setvbuf(stderr, buff_stderr, _IOLBF, sizeof(buff_stderr));
    setvbuf(stdout, buff_stdout, _IOLBF, sizeof(buff_stdout));
    
    // Make fgets not blocking 
    int flags = fcntl(stdin->_fileno, F_GETFL, 0); 
    fcntl(stdin->_fileno, F_SETFL, flags | O_NONBLOCK); 
    
    /* Make read and write ends of g_pfd nonblocking */
    pipe(g_pfd);
    flags = fcntl(g_pfd[0], F_GETFL);
    fcntl(g_pfd[0], F_SETFL, flags | O_NONBLOCK); /* Make read end nonblocking */
    
    flags = fcntl(g_pfd[1], F_GETFL);
    fcntl(g_pfd[1], F_SETFL, flags | O_NONBLOCK); /* Make write end nonblocking */
}

static StrPair_t** GetHttpHeaderFields(int argc, char *argv[])
{
    StrPair_t **headerFields = calloc(argc+1, sizeof(StrPair_t*));
    int  i = 0;
    int  hIdx = 0;
    int  size = 0;
    char *ptr = 0;
    
    for(i=0; i<argc; ++i)
    {
        ptr = strchr(argv[i], '=');
        if(ptr)
        {
            headerFields[hIdx] = calloc(1, sizeof(StrPair_t));
            
            /* key */
            size = ptr - argv[i];
            headerFields[hIdx]->pKey = calloc(size, sizeof(char));
            strncpy(headerFields[hIdx]->pKey, argv[i], size);
            
            /* val */
            size = strlen(ptr+1);
            headerFields[hIdx]->pVal = calloc(size+1, sizeof(char));
            strncpy(headerFields[hIdx]->pVal, ptr+1, size);
            
            ++hIdx;
        }
    }
    return headerFields;
}

static int HandleTracks(const char *argvBuff)
{
    int commandRetVal = 0;
    
    switch (argvBuff[1]) 
    {
        case 'l': 
        {
            int num = 0;
            TrackDescription_t *TrackList = backend_get_tracks_list(argvBuff[0], &num);
            break;
        }
        case 'c': 
        {
            TrackDescription_t *track = backend_get_current_track(argvBuff[0]);
            break;
        }
        default: 
        {
            /* switch command available only for audio tracks */
            if ('a' == argvBuff[0])
            {
                int id = -1;
                sscanf(argvBuff+1, "%d", &id);
                if(id >= 0)
                {
                    commandRetVal = backend_set_track(argvBuff[0], id);
                    fprintf(stderr, "{\"%c_%c\":{\"id\":%d,\"sts\":%d}}\n", argvBuff[0], 's', id, commandRetVal);
                }
            }
            break;
        }
    }
    
    return commandRetVal;
}

int main(int argc,char* argv[]) 
{
    char argvBuff[1024];
    memset(argvBuff, '\0', sizeof(argvBuff));

    int audioTrackIdx = -1;
    int commandRetVal = -1;
    int retCode = 0;
    
    gchar *filename = NULL;
    
    gchar *downloadBufferPath = NULL;
    guint64 ringBufferMaxSize = 0;
    
    gint64 bufferDuration = -1;
    gint bufferSize = 0;
    
    StrPair_t **pHeaderFields = NULL;
    
    printf("%s >\n", __FILE__);
    signal(SIGINT, SigHandler);
    /* inform client that we can handle additional commands */
#if GST_VERSION_MAJOR < 1
    int ver = 20;
#else
    int ver = 10021;
#endif
    fprintf(stderr, "{\"GSTPLAYER_EXTENDED\":{\"version\":%d,\"gst_ver_major\":%d}}\n", ver, GST_VERSION_MAJOR);

    int usedArgs = 2;
    if(usedArgs > argc)
    {
        printf("Version for gstreamer %d.X\n", GST_VERSION_MAJOR);
        printf("give me a filename please\n");
        exit(1);
    }
    
    filename = g_strdup(argv[1]);
    
    if(argc>usedArgs)
    {
        audioTrackIdx = atoi(argv[usedArgs]);
    }
    ++usedArgs;
    
    if(argc>usedArgs)
    {
        backend_set_download_timeout( atoi(argv[usedArgs]) );
    }
    ++usedArgs;
    
    if(argc>usedArgs)
    {
        backend_set_is_live( atoi(argv[usedArgs]) );
    }
    ++usedArgs;
    
    if(argc>usedArgs)
    {
        downloadBufferPath = g_strdup(argv[usedArgs]);
    }
    ++usedArgs;
    
    if(argc>usedArgs)
    {
        sscanf(argv[usedArgs], "%llu", &ringBufferMaxSize);
    }
    ++usedArgs;
    
    if(argc>usedArgs)
    {
        sscanf(argv[usedArgs], "%lld", &bufferDuration);
    }
    ++usedArgs;
    
    if(argc>usedArgs)
    {
        sscanf(argv[usedArgs], "%d", &bufferSize);
    }
    
    if(argc>(usedArgs+1) && !strncmp(filename, "http", 4))
    {
        pHeaderFields = GetHttpHeaderFields(argc-usedArgs, argv+usedArgs); 
    }
    
    InitInOut();
    backend_init(&argc, &argv, g_pfd[1]);
    commandRetVal = backend_play(filename, downloadBufferPath, ringBufferMaxSize, bufferDuration, bufferSize, pHeaderFields);
    fprintf(stderr, "{\"PLAYBACK_PLAY\":{\"file\":\"%s\", \"sts\":%d}}\n", argv[1], commandRetVal);

    if(0 == commandRetVal) 
    {
        while(!g_terminated && backend_get_playback_info()->isPlaying)
        {
            if (audioTrackIdx >= 0)
            {
                static char cmd[128] = ""; // static to not allocate on stack
                sprintf(cmd, "a%d\n", audioTrackIdx);
                HandleTracks(cmd);
                audioTrackIdx = -1;
            }
            
            backend_gst_poll();
 
            /* We made fgets non blocking, so it will return immediately when there is nothing to read */
            if( NULL == fgets(argvBuff, sizeof(argvBuff)-1 , stdin) )
            {
                /* Wait for input command or for gst message - max 1s */
                WaitForCommand();
                continue;
            }

            if(0 == argvBuff[0])
            {
                continue;
            }
            
            switch(argvBuff[0])
            {
            case 'v':
            case 'a': 
            {
                HandleTracks(argvBuff);
                break;
            }
            case 'q':
            {
                commandRetVal = backend_stop();
                fprintf(stderr, "{\"PLAYBACK_STOP\":{\"sts\":%d}}\n", commandRetVal);
                break;
            }
            case 'c':
            {
                commandRetVal = backend_resume();
                fprintf(stderr, "{\"PLAYBACK_CONTINUE\":{\"sts\":%d}}\n", commandRetVal);
                break;
            }
            case 'p':
            {
                commandRetVal = backend_pause();
                fprintf(stderr, "{\"PLAYBACK_PAUSE\":{\"sts\":%d}}\n", commandRetVal);
                break;
            }
            case 'm':
            {
            }
            case 'f':
            {
                int speed = 0;
                sscanf(argvBuff+1, "%d", &speed);

                commandRetVal = backend_set_speed(speed);
                fprintf(stderr, "{\"PLAYBACK_FASTFORWARD\":{\"speed\":%d, \"sts\":%d}}\n", speed, commandRetVal);
                break;
            }
            case 'b': 
            {
            }
            case 'g':
            {
                int gotoPos    = 0;
                double length  = 0.0;
                int lengthInt  = 0;
                double seconds = 0.0;
                unsigned char force = ('f' == argvBuff[1]) ? 1 : 0; // f - force, c - check
                
                sscanf(argvBuff+2, "%d", &gotoPos);
                if(0 <= gotoPos || force)
                {
                    commandRetVal = backend_query_duration(&length);
                    fprintf(stderr, "{\"PLAYBACK_LENGTH\":{\"length\":%lf, \"sts\":%d}}\n", length, commandRetVal);

                    lengthInt = (int)length;
                    if(10 <= lengthInt || force)
                    {
                        seconds = gotoPos;
                        if(!force && gotoPos >= lengthInt)
                        {
                            seconds = lengthInt - 10;
                        }
                        
                        commandRetVal = backend_seek_absolute(seconds);
                        fprintf(stderr, "{\"PLAYBACK_SEEK_ABS\":{\"sec\":%f, \"sts\":%d}}\n", seconds, commandRetVal);
                    }
                }
                break;
            }
            case 'k': 
            {
                int seek = 0;
                double length = 0;
                int lengthInt = 0;
                double seconds = 0;
                int64_t currentMSec = 0;
                unsigned char force = ('f' == argvBuff[1]) ? 1 : 0; // f - force, c - check
                
                sscanf(argvBuff+2, "%d", &seek);
                
                commandRetVal = backend_query_position(&currentMSec);
                if(0 == commandRetVal)
                {
                    fprintf(stderr, "{\"J\":{\"ms\":%lld}}\n", currentMSec);
                }
                if(0 == commandRetVal || force)
                {
                    int CurrentSec = (int) (currentMSec/1000);
                    
                    commandRetVal = backend_query_duration(&length);
                    fprintf(stderr, "{\"PLAYBACK_LENGTH\":{\"length\":%lf, \"sts\":%d}}\n", length, commandRetVal);
                    
                    lengthInt = (int)length;
                    if(10 <= lengthInt || force )
                    {
                        int ergSec = CurrentSec + seek;
                        if(!force && 0 > ergSec)
                        {
                            seconds = 0; //CurrentSec * -1; // jump to start position
                        }
                        else if(!force && ergSec >= lengthInt)
                        {
                            seconds = lengthInt - 5; //(lengthInt - CurrentSec) - 5;
                            if(0 < seconds)
                            {
                                seconds = CurrentSec; //0; // no jump we are at the end
                            }
                        }
                        else
                        {
                            seconds = ergSec; //seek;
                        }
                    }
                    commandRetVal = backend_seek_absolute(seconds); //backend_seek(seconds);
                    fprintf(stderr, "{\"PLAYBACK_SEEK\":{\"sec\":%lf, \"sts\":%d}}\n", seconds, commandRetVal);
                }
                break;
            }
            case 'l': 
            {
                /*
                double length = 0;
                commandRetVal = backend_query_duration(&length);
                fprintf(stderr, "{\"PLAYBACK_LENGTH\":{\"length\":%lf, \"sts\":%d}}\n", length, commandRetVal);
                */
                break;
            }
            case 'j': 
            {
                int64_t currentMSec = 0;
                commandRetVal = backend_query_position(&currentMSec);
                if (0 == commandRetVal)
                {
                    fprintf(stderr, "{\"J\":{\"ms\":%lld}}\n", currentMSec);
                }
                break;
            }

            case 'i':
            {
                // PlaybackHandler_t *ptrP = player->playback;
                // if(ptrP)
                // {
                    // fprintf(stderr, "{\"PLAYBACK_INFO\":{ \"isPlaying\":%s, \"isPaused\":%s, \"isForwarding\":%s, \"isSeeking\":%s, \"isCreationPhase\":%s,", \
                    // DUMP_BOOL(ptrP->isPlaying), DUMP_BOOL(ptrP->isPaused), DUMP_BOOL(ptrP->isForwarding), DUMP_BOOL(ptrP->isSeeking), DUMP_BOOL(ptrP->isCreationPhase) );
                    // fprintf(stderr, "\"BackWard\":%f, \"SlowMotion\":%d, \"Speed\":%d, \"AVSync\":%d,", ptrP->BackWard, ptrP->SlowMotion, ptrP->Speed, ptrP->AVSync);
                    // fprintf(stderr, " \"isVideo\":%s, \"isAudio\":%s, \"isSubtitle\":%s, \"isDvbSubtitle\":%s, \"isTeletext\":%s, \"mayWriteToFramebuffer\":%s, \"abortRequested\":%s }}\n", \
                    // DUMP_BOOL(ptrP->isVideo), DUMP_BOOL(ptrP->isAudio), DUMP_BOOL(ptrP->isSubtitle), DUMP_BOOL(ptrP->isDvbSubtitle), DUMP_BOOL(ptrP->isTeletext), DUMP_BOOL(ptrP->mayWriteToFramebuffer), DUMP_BOOL(ptrP->abortRequested) );
                // }
                break;
            }
            case 't':
            {
                uint64_t timeout = 0;
                if( 1 == sscanf(argvBuff+1, "%llu", &timeout))
                {
                    backend_set_download_timeout(timeout);
                }
                break;
            }
            default: 
            {
                break;
            }
            }
        }
    }
    else
    {
        retCode = -10;
    }
    
    backend_deinit();
    
    if(-1 < g_pfd[0])
    {
        close(g_pfd[0]);
    }
    if(-1 < g_pfd[1])
    {
        close(g_pfd[1]);
    }
    
    if(pHeaderFields)
    {
        g_free(filename);
        if(downloadBufferPath)
        {
            g_free(downloadBufferPath);
        }
        if(pHeaderFields)
        {
            /* free pHeaderFields */
            /* this is not need, memory will be free by the system, after process exit */
        }
    }
    
    exit(retCode);
}
