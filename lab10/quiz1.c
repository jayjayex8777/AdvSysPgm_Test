#include "chardev.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* open */
#include <unistd.h>             /* exit */
#include <sys/ioctl.h>          /* ioctl */
#include <sys/types.h>

int main()
{
        int file_desc;
        char *buf = (char *)malloc(8192);

        file_desc = open(DEVICE_FILE_NAME, 0);
        if (file_desc < 0) {
                printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
                exit(-1);
        }

        ioctl(file_desc, IOCTL_PROCINFO);

        free(buf);
        close(file_desc);
}
