#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>


#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

static dev_t dev;
static struct class *dev_class;
// static struct cdev test_ioctl_cdev;

static int major;

#define MAX_SIZE 128
#define DEVICE_NAME "onewire"

static ssize_t onewire_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    return 0;
}

static ssize_t onewire_write(struct file *filp, const char __user *buff, size_t len, loff_t *off) {

    // char buffer[MAX_SIZE];
    char* buffer = (char*) kzalloc(sizeof(char)*len, GFP_KERNEL);

    // size_t buffer_size = min(MAX_SIZE, len);
    if(copy_from_user(buffer, buff, len)) // 20 works
        return -1;
        // return-EFAULT;

    // if(len < MAX_SIZE) {
    //     printk("Received from user %lu \n", len);
    // }
    printk("Received from user %lu \n", len);

    kfree(buffer);

    return len;
}


static long onewire_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // Implement ioctl operation
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = onewire_read,
    .write = onewire_write,
    .unlocked_ioctl = onewire_ioctl,
};

    // if (alloc_chrdev_region(&dev, 0, 1, "onewire") < 0) {
    //     printk(KERN_ERR "Failed to allocate char device\n");
    //     return -1;
    // }

    // dev_class = class_create(THIS_MODULE, "onewire");
    // if (IS_ERR(dev_class)) {
    //     unregister_chrdev_region(dev, 1);
    //     printk(KERN_ERR "Failed to create char device class\n");
    //     return PTR_ERR(dev_class);
    // }
    // device_create(dev_class, NULL, dev, NULL, "onewire");
    // printk(KERN_INFO "My Device Character Driver Loaded\n");
    // return 0;

static int __init onewire_init(void) {

//     dev_t dev;
//     int alloc_ret =-1;
//     int cdev_ret =-1;
//     int num_of_dev = 1;
//     alloc_ret = alloc_chrdev_region(&dev, 0, num_of_dev, "onewire");

//     if(alloc_ret)
//         goto error;
//     int test_ioctl_major = MAJOR(dev);
//     cdev_init(&test_ioctl_cdev, &fops);
//     cdev_ret = cdev_add(&test_ioctl_cdev, dev, num_of_dev);

//     if(cdev_ret)
//         goto error;

//     pr_alert("%s driver(major: %d) installed.\n", "onewire", test_ioctl_major);
//     return 0;

// error:
//     if(cdev_ret == 0)
//         cdev_del(&test_ioctl_cdev);
//     if(alloc_ret == 0)
//         unregister_chrdev_region(dev, num_of_dev);
    
//     return -1;
    major = register_chrdev(0, DEVICE_NAME, &fops);

    if(major < 0) {
        pr_alert("Registering char device failed with %d\n", major);
        return major;
    }

    pr_info("I was assigned major number %d.\n", major);

    dev_class= class_create(DEVICE_NAME);

    device_create(dev_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    pr_info("Device created on /dev/%s\n", DEVICE_NAME);

    return 0;
}

static void __exit onewire_exit(void) {
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "My Device Character Driver Unloaded\n");
}

module_init(onewire_init);
module_exit(onewire_exit);

MODULE_LICENSE("GPL");