#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>

#define DRIVER_NAME "tcs3472_driver"

// Địa chỉ thanh ghi dữ liệu bắt đầu từ CLEAR_LOW
#define TCS3472_REG_CDATAL 0x14

// Biến lưu client I2C
static struct i2c_client *tcs3472_client = NULL;

// Hàm đọc giá trị màu: 0=Clear, 1=Red, 2=Green, 3=Blue
int tcs3472_read_color(int color_index)
{
    u8 buf[8];
    s16 color_data[4];

    if (!tcs3472_client)
        return -ENODEV;

    if (i2c_smbus_read_i2c_block_data(tcs3472_client, TCS3472_REG_CDATAL, sizeof(buf), buf) < 0) {
        pr_err("tcs3472: Failed to read color data\n");
        return -EIO;
    }

    color_data[0] = (buf[1] << 8) | buf[0]; // Clear
    color_data[1] = (buf[3] << 8) | buf[2]; // Red
    color_data[2] = (buf[5] << 8) | buf[4]; // Green
    color_data[3] = (buf[7] << 8) | buf[6]; // Blue

    if (color_index < 0 || color_index > 3)
        return -EINVAL;

    return color_data[color_index];
}
EXPORT_SYMBOL(tcs3472_read_color); // Cho phép gọi từ module kernel khác nếu cần

// Hàm probe khi driver được gán cho thiết bị I2C
static int tcs3472_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    tcs3472_client = client;
    pr_info("tcs3472: Probed successfully\n");
    return 0;
}

// Hàm remove khi driver bị gỡ
static int tcs3472_remove(struct i2c_client *client)
{
    tcs3472_client = NULL;
    pr_info("tcs3472: Removed\n");
    return 0;
}

// Thông tin thiết bị dùng cho Device Tree (nếu có)
static const struct of_device_id tcs3472_of_match[] = {
    { .compatible = "invensense,tcs3472" },
    { },
};
MODULE_DEVICE_TABLE(of, tcs3472_of_match);

// ID table cho I2C core
static const struct i2c_device_id tcs3472_id[] = {
    { "tcs3472", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tcs3472_id);

// Định nghĩa driver I2C
static struct i2c_driver tcs3472_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = of_match_ptr(tcs3472_of_match),
        .owner = THIS_MODULE,
    },
    .probe = tcs3472_probe,
    .remove = tcs3472_remove,
    .id_table = tcs3472_id,
};

// Hàm khởi tạo và gỡ module
static int __init tcs3472_init(void)
{
    pr_info("tcs3472: Initializing driver\n");
    return i2c_add_driver(&tcs3472_driver);
}

static void __exit tcs3472_exit(void)
{
    pr_info("tcs3472: Exiting driver\n");
    i2c_del_driver(&tcs3472_driver);
}

module_init(tcs3472_init);
module_exit(tcs3472_exit);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel-only TCS3472 I2C Driver");
MODULE_LICENSE("GPL");
