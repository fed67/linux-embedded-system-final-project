#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include <linux/platform_device.h>      /* For platform devices */
#include <linux/gpio/consumer.h>        /* For GPIO Descriptor interface */
// #include <linux/interrupt.h>            /* For IRQ */
#include <linux/of.h>                   /* For DT*/
#include <linux/delay.h>

#define MAX_SIZE 128

#define MODULE_NAME "onewire_dev"

// Structure to hold device-specific data
struct onewire_dev {
    char *kernel_buffer;
    int buffer_size;
    struct cdev cdev;
};

struct gpio_desc *my_led;
static struct onewire_dev *s_dev = NULL;
static int major_number = 0;
static dev_t dev_num;

static struct class *cls;

// Define file operation functions
static int onewire_open(struct inode *inode, struct file *filp)
{
    struct onewire_dev *dev;
    dev = container_of(inode->i_cdev, struct onewire_dev, cdev);
    filp->private_data = dev; // Store device-specific data in file pointer
    printk(KERN_INFO "%s: Device opened\n", MODULE_NAME);

    return 0;
}

static int onewire_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "%s: Device released\n", MODULE_NAME);
    return 0;
}

static ssize_t onewire_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    // struct onewire_dev *dev = (struct onewire_dev *)filp->private_data;
    ssize_t bytes_read = 0;

    // if (*f_pos >= dev->buffer_size) {
    //     return 0; // End of file
    // }

    if (count > s_dev->buffer_size - *f_pos) {
        count = s_dev->buffer_size - *f_pos;
    }

    if (copy_to_user(buf, s_dev->kernel_buffer + *f_pos, count)) {
        return -EFAULT; // Failed to copy to user space
    }

    *f_pos += count;
    bytes_read = count;
    printk(KERN_INFO "%s: Read %zu bytes from device (offset: %lld)\n", MODULE_NAME, count, *f_pos);
    return bytes_read;
}

static ssize_t onewire_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    // struct onewire_dev *dev = (struct onewire_dev *)filp->private_data;
    ssize_t bytes_written = 0;

    pr_info("copy count %u f_pos %lu \n", count, *f_pos);
    pr_info("dev->buffer_size %lu \n", s_dev->buffer_size);


    // if (*f_pos >= dev->buffer_size) {
    //     return -ENOSPC; // No space left
    // }

    if (count > s_dev->buffer_size - *f_pos) {
        count = s_dev->buffer_size - *f_pos;
    }

    if (copy_from_user(s_dev->kernel_buffer + *f_pos, buf, count)) {
        return -EFAULT; // Failed to copy from user space
    }

    pr_info("copy_from user space scceeded\n");
    *f_pos += count;
    bytes_written = count;

    pr_info("copy_from user space scceeded\n");

    
    if(count > 0) {
        pr_info("value");
        int value = s_dev->kernel_buffer[0] - '0';
        pr_info("value %i\n", value);
        gpiod_set_value(my_led, value);
    }

    printk(KERN_INFO "%s: Wrote %zu bytes to device (offset: %lld)\n", MODULE_NAME, count, *f_pos);
    return bytes_written;
}

// File operations structure
static struct file_operations fops = {
    .open = onewire_open,
    .release = onewire_release,
    .read = onewire_read,
    .write = onewire_write,
};



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


    //initialize character device


    // ret = alloc_chrdev_region(&dev_num, 0, 1, MODULE_NAME);
    // if (ret < 0) {
    //     printk(KERN_ERR "%s: Failed to allocate character device region\n", MODULE_NAME);
    //     return ret;
    // }


    s_dev = kmalloc(sizeof(struct onewire_dev), GFP_KERNEL);
    if (!s_dev) {
        ret = -ENOMEM;
        printk(KERN_ERR "%s: Failed to allocate device structure\n", MODULE_NAME);
        goto unregister_region;
    }
    memset(s_dev, 0, sizeof(struct onewire_dev));

    // Allocate kernel buffer
    // s_dev->buffer_size = PAGE_SIZE; // Use page size for buffer
    s_dev->buffer_size = 512; // Use page size for buffer
    s_dev->kernel_buffer = kmalloc(s_dev->buffer_size, GFP_KERNEL);
    if (!s_dev->kernel_buffer) {
        ret = -ENOMEM;
        printk(KERN_ERR "%s: Failed to allocate kernel buffer\n", MODULE_NAME);
        goto free_device_struct;
    }
    memset(s_dev->kernel_buffer, 0, sizeof(s_dev->buffer_size));

    // Initialize the character device
    major_number = register_chrdev(0, MODULE_NAME, &fops);
    if(major_number < 0) {
        pr_alert("Registering char device failed with %d\n", major_number);
        goto free_kernel_buffer;
    }
 
    pr_info("I was assigned major number %d.\n", major_number);
 
    cls= class_create(MODULE_NAME);

    device_create(cls, NULL, MKDEV(major_number, 0), NULL, MODULE_NAME);
 
    pr_info("Device created on /dev/%s\n", MODULE_NAME);
    
    return 0;

free_kernel_buffer:
    kfree(s_dev->kernel_buffer);
free_device_struct:
    kfree(s_dev);
unregister_region:
    unregister_chrdev_region(dev_num, 1);
    return ret;
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


    device_destroy(cls, MKDEV(major_number, 0));
    class_destroy(cls);

    //unregister
    unregister_chrdev(major_number, MODULE_NAME);

    return 0;
}


static const struct of_device_id my_of_match[] = {
    { .compatible = "my-onewire" }, //brcm,bcm2835-gpio
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

// // Initialization function
// static int __init onewire_dev_init(void)
// {
//     int result;

//     result = alloc_chrdev_region(&dev_num, 0, 1, MODULE_NAME);
//     if (result < 0) {
//         printk(KERN_ERR "%s: Failed to allocate character device region\n", MODULE_NAME);
//         return result;
//     }
//     major_number = MAJOR(dev_num);
//     printk(KERN_INFO "%s: Major number allocated: %d\n", MODULE_NAME, major_number);

//     s_dev = kmalloc(sizeof(struct onewire_dev), GFP_KERNEL);
//     if (!s_dev) {
//         result = -ENOMEM;
//         printk(KERN_ERR "%s: Failed to allocate device structure\n", MODULE_NAME);
//         goto unregister_region;
//     }
//     memset(s_dev, 0, sizeof(struct onewire_dev));

//     // Allocate kernel buffer
//     s_dev->buffer_size = PAGE_SIZE; // Use page size for buffer
//     s_dev->kernel_buffer = kmalloc(s_dev->buffer_size, GFP_KERNEL);
//     if (!s_dev->kernel_buffer) {
//         result = -ENOMEM;
//         printk(KERN_ERR "%s: Failed to allocate kernel buffer\n", MODULE_NAME);
//         goto free_device_struct;
//     }

//     // Initialize the character device
//     cdev_init(&s_dev->cdev, &fops);
//     s_dev->cdev.owner = THIS_MODULE;

//     // Add the character device to the system
//     result = cdev_add(&s_dev->cdev, dev_num, 1);
//     if (result < 0) {
//         printk(KERN_ERR "%s: Failed to add character device\n", MODULE_NAME);
//         goto free_kernel_buffer;
//     }

//     printk(KERN_INFO "%s: Device registered successfully. Create device file using:\n", MODULE_NAME);
//     printk(KERN_INFO "      mknod /dev/%s c %d %d\n", MODULE_NAME, major_number, MINOR(dev_num));

//     return 0;

// free_kernel_buffer:
//     kfree(s_dev->kernel_buffer);
// free_device_struct:
//     kfree(s_dev);
// unregister_region:
//     unregister_chrdev_region(dev_num, 1);
//     return result;
// }

// // Exit function
// static void __exit onewire_dev_exit(void)
// {
//     cdev_del(&s_dev->cdev);
//     kfree(s_dev->kernel_buffer);
//     kfree(s_dev);
//     unregister_chrdev_region(dev_num, 1);
//     printk(KERN_INFO "%s: Device unregistered\n", MODULE_NAME);
// }

// module_init(onewire_dev_init);
// module_exit(onewire_dev_exit);


MODULE_LICENSE("GPL");
// MODULE_LICENSE("Proprietary");