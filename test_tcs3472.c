#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVICE_FILE "/dev/tcs3472"

// IOCTL định nghĩa khớp với kernel driver
#define TCS3472_IOCTL_MAGIC 'm'
#define TCS3472_IOCTL_READ_CLEAR _IOR(TCS3472_IOCTL_MAGIC, 1, int)
#define TCS3472_IOCTL_READ_RED   _IOR(TCS3472_IOCTL_MAGIC, 2, int)
#define TCS3472_IOCTL_READ_GREEN _IOR(TCS3472_IOCTL_MAGIC, 3, int)
#define TCS3472_IOCTL_READ_BLUE  _IOR(TCS3472_IOCTL_MAGIC, 4, int)
#define TCS3472_IOCTL_STREAM_COLOR _IO(TCS3472_IOCTL_MAGIC, 5)

int main() {
    int fd;
    int value;

    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    printf("Starting color stream (Press Ctrl+C to stop):\n");

    // Gọi ioctl để kích hoạt stream từ kernel
    if (ioctl(fd, TCS3472_IOCTL_STREAM_COLOR, NULL) < 0) {
        perror("Failed to stream color");
        close(fd);
        return 1;
    }

    // Không cần vòng lặp vì kernel đã stream bằng `printk`, xem bằng:
    // $ dmesg -w

    // Hoặc nếu bạn muốn hiển thị từng màu một cách thủ công:
    
    while (1) {
        ioctl(fd, TCS3472_IOCTL_READ_CLEAR, &value);
        printf("Clear: %d\n", value);

        ioctl(fd, TCS3472_IOCTL_READ_RED, &value);
        printf("Red: %d\n", value);

        ioctl(fd, TCS3472_IOCTL_READ_GREEN, &value);
        printf("Green: %d\n", value);

        ioctl(fd, TCS3472_IOCTL_READ_BLUE, &value);
        printf("Blue: %d\n", value);

        sleep(1);
    }
    

    close(fd);
    return 0;
}
