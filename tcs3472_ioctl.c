#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define DRIVER_NAME "tcs3472_driver"
#define CLASS_NAME "tcs3472"
#define DEVICE_NAME "tcs3472"

#define tcs3472_CLEAR_LOW 0x14

#define tcs3472_IOCTL_MAGIC 'm'
#define tcs3472_IOCTL_READ_CLEAR _IOR(tcs3472_IOCTL_MAGIC, 1, int)
#define tcs3472_IOCTL_READ_RED _IOR(tcs3472_IOCTL_MAGIC, 2, int)
#define tcs3472_IOCTL_READ_GREEN _IOR(tcs3472_IOCTL_MAGIC, 3, int)
#define tcs3472_IOCTL_READ_BLUE _IOR(tcs3472_IOCTL_MAGIC, 4, int)
#define TCS3472_IOCTL_STREAM_COLOR _IO(tcs3472_IOCTL_MAGIC, 5)

static struct i2c_client *tcs3472_client;
static struct class *tcs3472_class = NULL;
static struct device *tcs3472_device = NULL;
static struct task_struct *stream_thread = NULL;
static int major_number;

static int tcs3472_read_color(struct i2c_client *client, int color) {
    u8 buf[8];
    s16 color_data[4];

    if (i2c_smbus_read_i2c_block_data(client, tcs3472_CLEAR_LOW, sizeof(buf), buf) < 0) {
        printk(KERN_ERR "tcs3472: Failed to read color data\n");
        return -EIO;
    }

    color_data[0] = (buf[1] << 8) | buf[0]; // Clear
    color_data[1] = (buf[3] << 8) | buf[2]; // Red
    color_data[2] = (buf[5] << 8) | buf[4]; // Green
    color_data[3] = (buf[7] << 8) | buf[6]; // Blue

    return color_data[color];
}

static int tcs3472_stream_thread(void *data) {
    struct i2c_client *client = (struct i2c_client *)data;
    int clear, red, green, blue;

    while (!kthread_should_stop()) {
        clear = tcs3472_read_color(client, 0);
        red = tcs3472_read_color(client, 1);
        green = tcs3472_read_color(client, 2);
        blue = tcs3472_read_color(client, 3);

        printk(KERN_INFO "tcs3472: Clear=%d, Red=%d, Green=%d, Blue=%d\n",
               clear, red, green, blue);

        msleep(500);
    }

    return 0;
}

static long tcs3472_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int data;

    if (!tcs3472_client) {
        printk(KERN_ERR "tcs3472: I2C client not initialized\n");
        return -EFAULT;
    }

    switch (cmd) {
        case TCS3472_IOCTL_STREAM_COLOR:
            if (!stream_thread) {
                stream_thread = kthread_run(tcs3472_stream_thread, tcs3472_client, "tcs3472_stream");
                if (IS_ERR(stream_thread)) {
                    printk(KERN_ERR "tcs3472: Failed to create stream thread\n");
                    stream_thread = NULL;
                    return PTR_ERR(stream_thread);
                }
            } else {
                printk(KERN_INFO "tcs3472: Stream thread already running\n");
            }
            break;

        case tcs3472_IOCTL_READ_CLEAR:
            data = tcs3472_read_color(tcs3472_client, 0);
            goto copy_data;

        case tcs3472_IOCTL_READ_RED:
            data = tcs3472_read_color(tcs3472_client, 1);
            goto copy_data;

        case tcs3472_IOCTL_READ_GREEN:
            data = tcs3472_read_color(tcs3472_client, 2);
            goto copy_data;

        case tcs3472_IOCTL_READ_BLUE:
            data = tcs3472_read_color(tcs3472_client, 3);
            goto copy_data;

        default:
            return -EINVAL;
    }

    return 0;

copy_data:
    if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
        return -EFAULT;
    }
    return 0;
}

static int tcs3472_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "tcs3472: Device opened\n");
    return 0;
}

static int tcs3472_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "tcs3472: Device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = tcs3472_open,
    .unlocked_ioctl = tcs3472_ioctl,
    .release = tcs3472_release,
};

static int tcs3472_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    tcs3472_client = client;

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "tcs3472: Failed to register major number\n");
        return major_number;
    }

    tcs3472_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(tcs3472_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(tcs3472_class);
    }

    tcs3472_device = device_create(tcs3472_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(tcs3472_device)) {
        class_destroy(tcs3472_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(tcs3472_device);
    }

    printk(KERN_INFO "tcs3472: Driver installed\n");
    return 0;
}

static int tcs3472_remove(struct i2c_client *client) {
    if (stream_thread) {
        kthread_stop(stream_thread);
        stream_thread = NULL;
    }

    device_destroy(tcs3472_class, MKDEV(major_number, 0));
    class_unregister(tcs3472_class);
    class_destroy(tcs3472_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "tcs3472: Driver removed\n");
    return 0;
}

static const struct of_device_id tcs3472_of_match[] = {
    { .compatible = "invensense,tcs3472", },
    { },
};
MODULE_DEVICE_TABLE(of, tcs3472_of_match);

static const struct i2c_device_id tcs3472_id[] = {
    { "tcs3472", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tcs3472_id);

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

static int __init tcs3472_init(void) {
    printk(KERN_INFO "tcs3472: Initializing driver\n");
    return i2c_add_driver(&tcs3472_driver);
}

static void __exit tcs3472_exit(void) {
    printk(KERN_INFO "tcs3472: Exiting driver\n");
    i2c_del_driver(&tcs3472_driver);
}

module_init(tcs3472_init);
module_exit(tcs3472_exit);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("TCS3472 I2C Driver with IOCTL and Stream");
MODULE_LICENSE("GPL");
