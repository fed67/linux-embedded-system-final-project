#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>      /* For platform devices */
#include <linux/gpio/consumer.h>        /* For GPIO Descriptor interface */
// #include <linux/interrupt.h>            /* For IRQ */
#include <linux/of.h>                   /* For DT*/
#include <linux/delay.h>


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
static struct gpio_desc *my_led = NULL;


static int onewire_probe(struct platform_device *pdev)
{
    pr_info("onewire:  onewire_probe");

    struct device *dev = &pdev->dev;
	const char *label;
	int my_value, ret;

	printk("dt_gpio - Now I am in the probe function!\n");

	/* Check for device properties */
	if(!device_property_present(dev, "label")) {
		printk("dt_gpio - Error! Device property 'label' not found!\n");
		return -1;
	}
	if(!device_property_present(dev, "my_value")) {
		printk("dt_gpio - Error! Device property 'my_value' not found!\n");
		return -1;
	}
	if(!device_property_present(dev, "green-led-gpio")) {
		printk("dt_gpio - Error! Device property 'green-led-gpio' not found!\n");
		return -1;
	}

	/* Read device properties */
	ret = device_property_read_string(dev, "label", &label);
	if(ret) {
		printk("dt_gpio - Error! Could not read 'label'\n");
		return -1;
	}
	printk("dt_gpio - label: %s\n", label);
	ret = device_property_read_u32(dev, "my_value", &my_value);
	if(ret) {
		printk("dt_gpio - Error! Could not read 'my_value'\n");
		return -1;
	}
	printk("dt_gpio - my_value: %d\n", my_value);

	/* Init GPIO */
	my_led = gpiod_get(dev, "green-led", GPIOD_OUT_LOW);
	if(IS_ERR(my_led)) {
		printk("dt_gpio - Error! Could not setup the GPIO\n");
		return -1 * IS_ERR(my_led);
	}

    ret = gpiod_get_value(my_led);
    printk("get value 1 %d\n", ret);
    ret = gpiod_get_raw_value(my_led);
    printk("get raw value 1 %d\n", ret);

    gpiod_set_value(my_led, 1);

    udelay(10);

    ret = gpiod_get_value(my_led);
    printk("get value 2 %d\n", ret);
    ret = gpiod_get_raw_value(my_led);
    printk("get raw value 2 %d\n", ret);
    // gpiod_set_value(my_led, 0); // turn on LED (if active low)


    // return 0;

    // // Now you can control it
    // gpiod_set_value(data->status_led, 1); // turn on LED (if active low)

    // platform_set_drvdata(pdev, data);

    // pr_info("dummy char module loaded\n");
    return 0;
}


//21
static int onewire_remove(struct platform_device *pdev)
{
    pr_info("onewire:  onewire_remove");
    // gpiod_put(red);
    // gpiod_put(green);
    // struct my_driver_data *data = platform_get_drvdata(pdev);

    // gpiod_set_value(data->status_led, 0); // turn off LED on remove

    int ret = gpiod_get_value(my_led);
    printk("get value 1 %d\n", ret);
    ret = gpiod_get_raw_value(my_led);
    printk("get raw value 1 %d\n", ret);

    gpiod_set_value(my_led, 0);
    udelay(10);
    
    ret = gpiod_get_value(my_led);
    printk("get value 2 %d\n", ret);
    ret = gpiod_get_raw_value(my_led);
    printk("get raw value 2 %d\n", ret);

    gpiod_put(my_led);
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