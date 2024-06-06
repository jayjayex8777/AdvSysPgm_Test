#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

int main(int argc, char **argv)
{
        int fd;
        fd = open("/proc/mmap_test", O_RDWR);
        if (fd < 0) {
                perror("open");
                return -1;
        }

        char *mem = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mem == MAP_FAILED) {
                perror("mmap");
                return -1;
        }

        sleep(1);

        printf("initial message: \"%s\"\n", mem);
        printf("changed message: \"%s\"\n", mem);

        close(fd);
        munmap(mem, PAGE_SIZE);

        sleep(1);

        return 0;
}
