#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>

#define WR_VALUE _IOW('a', 'a', int32_t *)
#define RD_VALUE _IOR('a', 'b', int32_t *)

int main()
{
    int fd;
    int32_t value = 123;

    fd = open("/dev/mydevice", O_RDWR);

    ioctl(fd, WR_VALUE, &value);

    value = 0;

    ioctl(fd, RD_VALUE, &value);

    printf("Value = %d\n", value);

    close(fd);

    return 0;
}
