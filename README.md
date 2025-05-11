# TCS3472 Kernel Driver

Driver nhân Linux cho cảm biến màu TCS3472 sử dụng giao tiếp I2C trên Raspberry Pi.

## Tính năng

- Đọc giá trị màu: Clear, Red, Green, Blue
- Giao tiếp thông qua ioctl
- Hỗ trợ stream màu liên tục (dmesg)

## Cài đặt

### Build driver:

```bash
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
sudo insmod tcs3472_ioctl.ko
