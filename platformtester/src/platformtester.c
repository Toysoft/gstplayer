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

int main(int argc, char *argv[])
{
    printf("Test platform OK\n");
    return 0;
}