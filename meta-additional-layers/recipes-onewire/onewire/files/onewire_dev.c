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

#include <linux/gpio/consumer.h>

static dev_t dev;
static struct class *onewire_class;
static struct device *onewire_device;

static struct cdev test_ioctl_cdev;
static unsigned int test_ioctl_major = 0;

// static int major;

#define MAX_SIZE 128
#define DEVICE_NAME "onewire_dev"

#define GPIO_PIN 17
static struct gpio_desc *io_pin;

struct onewire_device_data {
    struct gpio_desc *led_gpio;
    const char *label;
};

static int onewire_close(struct inode *inode, struct file *filp)
 {
    pr_alert("%s call.\n", __func__);
    return 0;
 }
 
 
 static int onewire_open(struct inode *inode, struct file *filp)
 {
    printk("onewire_dev open\n");
    io_pin = gpio_to_desc(GPIO_PIN);

    return 0;
 }

static ssize_t onewire_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    return 0;
}

static ssize_t onewire_write(struct file *filp, const char __user *buff, size_t len, loff_t *off) {

    printk("onewire driver\n");

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

static int onewireprobe(struct platform_device *pdev)
{
    struct my_gpio_device_data *dev_data;
    int ret;

    dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
    if (!dev_data)
        return -ENOMEM;

    /* Get the GPIO descriptor using the consumer interface and device tree */
    dev_data->led_gpio = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_INIT_LOW);
    if (IS_ERR(dev_data->led_gpio)) {
        dev_err(&pdev->dev, "Failed to get GPIO for 'led'\n");
        return PTR_ERR(dev_data->led_gpio);
    }

    /* Get the label property from the device tree (optional) */
    ret = of_property_read_string(pdev->dev.of_node, "label", &dev_data->label);
    if (ret < 0) {
        dev_info(&pdev->dev, "No 'label' property found in device tree\n");
        dev_data->label = "default-led";
    }

    dev_info(&pdev->dev, "Got GPIO %d (%s) for '%s'\n",
             gpiod_get_pin(dev_data->led_gpio), dev_data->label, dev_name(&pdev->dev));

    /* Example usage: Blink the LED */
    printk(KERN_INFO "%s: Starting to blink LED '%s'\n", DRIVER_NAME, dev_data->label);
    for (int i = 0; i < 5; i++) {
        gpiod_set_value(dev_data->led_gpio, 1); /* Turn LED on */
        msleep(500);
        gpiod_set_value(dev_data->led_gpio, 0); /* Turn LED off */
        msleep(500);
    }
    printk(KERN_INFO "%s: Finished blinking LED '%s'\n", DRIVER_NAME, dev_data->label);

    platform_set_drvdata(pdev, dev_data);
    return 0;
}

static int onewire_remove(struct platform_device *pdev)
{
    struct my_gpio_device_data *dev_data = platform_get_drvdata(pdev);

    dev_info(&pdev->dev, "Releasing GPIO for '%s'\n", dev_name(&pdev->dev));
    /* devm_gpiod_get automatically handles releasing the GPIO in the remove function */

    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = onewire_open,
    .release = onewire_close,
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
    int alloc_ret = -1;
    int cdev_ret = -1;
    int num_of_dev = 1;

    // // Request the GPIO pin
    // if (gpio_request(GPIO_PIN, DEVICE_NAME) < 0) {
    //     printk(KERN_ERR "Failed to request GPIO %d\n", GPIO_PIN);
    //     return -EBUSY;
    // }

    // // Set the GPIO as an output initially (you might want to configure direction based on your needs)
    // if (gpio_direction_output(GPIO_PIN, 0) < 0) {
    //     printk(KERN_ERR "Failed to set GPIO %d as output\n", GPIO_PIN);
    //     gpio_free(GPIO_PIN);
    //     return -EBUSY;
    // }

    alloc_ret = alloc_chrdev_region(&dev, 0, num_of_dev, DEVICE_NAME);
    if (alloc_ret)
        goto error;

    test_ioctl_major = MAJOR(dev);

    cdev_init(&test_ioctl_cdev, &fops);
    cdev_ret = cdev_add(&test_ioctl_cdev, dev, num_of_dev);
    if (cdev_ret)
        goto error;

    // --- Add these lines to create class and device ---
    onewire_class = class_create(DEVICE_NAME);
    if (IS_ERR(onewire_class)) {
        pr_err("Failed to create class\n");
        goto error;
    }

    onewire_device = device_create(onewire_class, NULL, dev, NULL, DEVICE_NAME);
    if (IS_ERR(onewire_device)) {
        pr_err("Failed to create device\n");
        class_destroy(onewire_class);
        goto error;
    }
    // --------------------------------------------------

    pr_alert("%s driver(major: %d) installed.\n", "onewire", test_ioctl_major);
    return 0;

error:
    if (cdev_ret == 0)
        cdev_del(&test_ioctl_cdev);
    if (alloc_ret == 0)
        unregister_chrdev_region(dev, num_of_dev);
    return -1;
}

static void __exit onewire_exit(void) {
    device_destroy(onewire_class, dev);
    class_destroy(onewire_class);
    cdev_del(&test_ioctl_cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "My Device Character Driver Unloaded\n");
}

module_init(onewire_init);
module_exit(onewire_exit);

MODULE_LICENSE("GPL");
// MODULE_LICENSE("Proprietary");