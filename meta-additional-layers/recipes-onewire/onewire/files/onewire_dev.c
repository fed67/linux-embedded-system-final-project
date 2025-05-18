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

#define GPIO_PIN 2
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
        // return PTR_ERR(data->status_led);
    }

    data->status_led = devm_gpiod_get(&pdev->dev, "gpios", GPIOD_OUT_LOW);
    if (IS_ERR(data->status_led)) {
        dev_err(&pdev->dev, "Failed to get status-led GPIO\n");
        return PTR_ERR(data->status_led);
    }

    return 0;

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
    { .compatible = "myvendor,my-gpio-led" }, //brcm,bcm2835-gpio
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

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
// MODULE_LICENSE("Proprietary");