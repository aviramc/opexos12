/*
 *
 *      This module allocates and frees memory from the system by mapping and unmapping annonymus files to virtual memory
 */

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MAPFILE "/dev/zero"



void *getCore(size_t size) {
    int fd;

    fd = open(MAPFILE, O_RDWR);

    if (fd == -1) {
        perror("Error opening file for writing");
        exit(-1);
    }

    void *p = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) {
        /* Q: Why isn't the fd closed here? Seems wrong */
        perror("Error mmapping the file");
        return NULL;
    }
    close(fd);
    return p;
}

void freeCore(void *p, size_t length){

    if (munmap(p, length) == -1) {

        perror("Error freeing mapped  memory");

    }
}

