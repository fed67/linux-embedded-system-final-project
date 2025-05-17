#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>      /* For platform devices */
#include <linux/gpio/consumer.h>        /* For GPIO Descriptor interface */
// #include <linux/interrupt.h>            /* For IRQ */
#include <linux/of.h>                   /* For DT*/



// static struct class *onewire_class;
// static struct device *onewire_device;

// static struct gpio_desc *red, *green;
// static int major;

#define MAX_SIZE 128
#define DEVICE_NAME "onewire_dev"

#define MODULE_NAME "onewire_dev"

#define GPIO_PIN 17
// static struct gpio_desc *io_pin;

// struct onewire_device_data {
//     struct gpio_desc *led_gpio;
//     const char *label;
// };

struct my_driver_data {
    struct gpio_desc *status_led;
};

static int onewire_probe(struct platform_device *pdev)
{
    pr_info("onewire:  onewire_probe");
    struct my_driver_data *data;
    // int ret; 

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->status_led = devm_gpiod_get(&pdev->dev, "status-led", GPIOD_OUT_LOW);
    if (IS_ERR(data->status_led)) {
        dev_err(&pdev->dev, "Failed to get status-led GPIO\n");
        return PTR_ERR(data->status_led);
    }

    // Now you can control it
    gpiod_set_value(data->status_led, 1); // turn on LED (if active low)

    platform_set_drvdata(pdev, data);

    pr_info("dummy char module loaded\n");
    return 0;
}


static int onewire_remove(struct platform_device *pdev)
{
    pr_info("onewire:  onewire_remove");
    // gpiod_put(red);
    // gpiod_put(green);
    struct my_driver_data *data = platform_get_drvdata(pdev);

    gpiod_set_value(data->status_led, 0); // turn off LED on remove
    pr_info("good bye reader!\n");
    // return 0;

    return 0;
}

// struct file_operations fops = {
//     .owner = THIS_MODULE,
//     .open = onewire_open,
//     .release = onewire_close,
//     .read = onewire_read,
//     .write = onewire_write,lsmod

//     .unlocked_ioctl = onewire_ioctl,
// };

static const struct of_device_id my_of_match[] = {
    { .compatible = "brcm,bcm2835-gpio" }, //brcm,bcm2835-gpio
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_of_match);

struct platform_driver my_driver = {
    .driver = {
        .name = MODULE_NAME,
        .of_match_table = my_of_match,
    },
    .probe = onewire_probe,
    .remove = onewire_remove,
};


//     alloc_ret = alloc_chrdev_region(&dev, 0, num_of_dev, DEVICE_NAME);
//     if (alloc_ret)
//         goto error;

//     test_ioctl_major = MAJOR(dev);

//     cdev_init(&test_ioctl_cdev, &fops);
//     cdev_ret = cdev_add(&test_ioctl_cdev, dev, num_of_dev);
//     if (cdev_ret)
//         goto error;

//     // --- Add these lines to create class and device ---
//     onewire_class = class_create(DEVICE_NAME);
//     if (IS_ERR(onewire_class)) {
//         pr_err("Failed to create class\n");
//         goto error;
//     }

//     onewire_device = device_create(onewire_class, NULL, dev, NULL, DEVICE_NAME);
//     if (IS_ERR(onewire_device)) {
//         pr_err("Failed to create device\n");
//         class_destroy(onewire_class);
//         goto error;
//     }
//     // --------------------------------------------------

//     pr_alert("%s driver(major: %d) installed.\n", "onewire", test_ioctl_major);
//     return 0;

// error:
//     if (cdev_ret == 0)
//         cdev_del(&test_ioctl_cdev);
//     if (alloc_ret == 0)
//         unregister_chrdev_region(dev, num_of_dev);
//     return -1;
// }

// static void __exit onewire_exit(void) {
//     device_destroy(onewire_class, dev);
//     class_destroy(onewire_class);
//     cdev_del(&test_ioctl_cdev);
//     unregister_chrdev_region(dev, 1);
//     printk(KERN_INFO "My Device Character Driver Unloaded\n");
// }

// module_init(onewire_init);
// module_exit(onewire_exit);

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
// MODULE_LICENSE("Proprietary");