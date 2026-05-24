#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

int main()
{
    int fd;
    struct pollfd pfd;

    fd = open("/dev/mydevice", O_RDONLY);

    pfd.fd = fd;
    pfd.events = POLLIN;

    printf("Waiting for data...\n");

    poll(&pfd, 1, -1);

    printf("Data available!\n");

    close(fd);

    return 0;
}
