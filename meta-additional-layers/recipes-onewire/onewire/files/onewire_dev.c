#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include <linux/gpio/consumer.h>   /* For GPIO Descriptor interface */
#include <linux/platform_device.h> /* For platform devices */
// #include <linux/interrupt.h>            /* For IRQ */
#include <linux/delay.h>
#include <linux/of.h> /* For DT*/

#include "constants.h"

#define MAX_SIZE 128

#define MODULE_NAME "onewire_dev"

// Structure to hold device-specific data
struct onewire_dev
{
    char *kernel_buffer;
    int buffer_size;
    struct cdev cdev;
};

struct gpio_desc *onewire_pin;
static struct onewire_dev *s_dev = NULL;
static int major_number = 0;
static dev_t dev_num;

static struct class *cls;


//Onewire functions

static void reset_onewire(void) {

    gpiod_direction_output(onewire_pin, GPIOD_OUT_HIGH_OPEN_DRAIN);
    gpiod_set_value(onewire_pin, GPIOD_OUT_LOW);
    usleep_range(tRSTL-5, tRSTL+5);
    gpiod_set_value(onewire_pin, GPIOD_OUT_HIGH_OPEN_DRAIN);


    gpiod_direction_input(onewire_pin);

    pr_info("Reset started \n");

    //Wait until device respons with low

    int flag = 0;
    for(int i = 0; i < (tPDH_max); ++i) {
        fsleep(1);
        if(gpiod_get_value(onewire_pin) == 0) {
            flag = 1;
            break;
        }
    }

    if(flag) {
        pr_info("reset from device detected \n");
    } else {
        pr_info("failed to receive reset ACK \n");
    }
}


static void transfer_bits(char* array, size_t length) {


    for(int i = 0; i < length; ++i) {
        char bit = 1;
        for(int k = 0; k < 8; k++) {

            
            char b = array[i] & k;
            if(!b) {
                gpiod_set_value(onewire_pin, b);
            }
            bit = bit << 1;

        }
    }

}


// Define file operation functions
static int
onewire_open (struct inode *inode, struct file *filp)
{
    struct onewire_dev *dev;
    dev = container_of (inode->i_cdev, struct onewire_dev, cdev);
    filp->private_data = dev; // Store device-specific data in file pointer
    printk (KERN_INFO "%s: Device opened\n", MODULE_NAME);

    return 0;
}

static int
onewire_release (struct inode *inode, struct file *filp)
{
    printk (KERN_INFO "%s: Device released\n", MODULE_NAME);
    return 0;
}

static ssize_t
onewire_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    // struct onewire_dev *dev = (struct onewire_dev *)filp->private_data;
    ssize_t bytes_read = 0;

    if (count > s_dev->buffer_size - *f_pos)
        {
            count = s_dev->buffer_size - *f_pos;
        }

    if (copy_to_user (buf, s_dev->kernel_buffer + *f_pos, count))
        {
            return -EFAULT; // Failed to copy to user space
        }

    *f_pos += count;
    bytes_read = count;
    printk (KERN_INFO "%s: Read %zu bytes from device (offset: %lld)\n",
            MODULE_NAME, count, *f_pos);
    return bytes_read;
}

static ssize_t
onewire_write (struct file *filp, const char __user *buf, size_t count,
               loff_t *f_pos)
{
    // struct onewire_dev *dev = (struct onewire_dev *)filp->private_data;
    ssize_t bytes_written = 0;

    pr_info ("copy count %u f_pos %llu \n", count, *f_pos);
    pr_info ("dev->buffer_size %u \n", s_dev->buffer_size);

    if (count > s_dev->buffer_size - *f_pos)
        {
            count = s_dev->buffer_size - *f_pos;
        }

    if (copy_from_user (s_dev->kernel_buffer + *f_pos, buf, count))
        {
            return -EFAULT; // Failed to copy from user space
        }

    pr_info ("copy_from user space scceeded\n");
    *f_pos += count;
    bytes_written = count;

    pr_info ("copy_from user space scceeded\n");

    char bits[2] = { 0b00110011, 0b00001111};

    if (count > 0)
        {
            switch(s_dev->kernel_buffer[0]) {

                case 'r':
                    reset_onewire();
                break;
                case 't':
                    transfer_bits(bits, 2);
                break;
                case 'i':
                    gpiod_direction_input(onewire_pin);
                    break;
                case '1':
                case '0':
                    // gpiod_direction_output(onewire_pin, GPIOD_OUT_HIGH_OPEN_DRAIN);
                    int value = s_dev->kernel_buffer[0] - '0';
                    pr_info ("value %i\n", value);
                    gpiod_set_value (onewire_pin, value);
                    break;
                case 'L':
                    // gpiod_direction_output(onewire_pin, GPIOD_OUT_HIGH_OPEN_DRAIN);
                    pr_info ("GPIOD_OUT_LOW_OPEN_DRAIN\n");
                    gpiod_set_value (onewire_pin, GPIOD_OUT_LOW_OPEN_DRAIN);
                    break;
                    break;
                case 'H':
                    // gpiod_direction_output(onewire_pin, GPIOD_OUT_HIGH_OPEN_DRAIN);
                    pr_info ("GPIOD_OUT_HIGH_OPEN_DRAIN\n");
                    gpiod_set_value (onewire_pin, GPIOD_OUT_HIGH_OPEN_DRAIN);
                    break;
                default:
            }

            pr_info ("value");
        }

    printk (KERN_INFO "%s: Wrote %zu bytes to device (offset: %lld)\n",
            MODULE_NAME, count, *f_pos);
    return bytes_written;
}

// File operations structure
static struct file_operations fops = {
    .open = onewire_open,
    .release = onewire_release,
    .read = onewire_read,
    .write = onewire_write,
};

static int
onewire_probe (struct platform_device *pdev)
{
    pr_info ("onewire: onewire_probe");

    struct device *dev = &pdev->dev;
    const char *label;

    //checking if the device haas the property label
    if (!device_property_present (dev, "label"))
        {
            pr_err ("Device property 'label' not found!\n");
            return -1;
        }

    //checking if the device haas the property onewire-gpio
    if (!device_property_present (dev, "onewire-gpio"))
        {
            pr_crit ("Device property 'onewire-gpio' not found!\n");
            return -1;
        }

    //reading the device property lable in the device tree
    int ret = device_property_read_string (dev, "label", &label);
    if (ret)
        {
            pr_err ("dt_gpio - Error! Could not read 'label'\n");
            return -1;
        }
    pr_info ("dt_gpio - label: %s\n", label);

    onewire_pin = gpiod_get (dev, "onewire", GPIOD_OUT_HIGH_OPEN_DRAIN);
    if (IS_ERR (onewire_pin))
        {
            pr_err ("gpiod_get error %d\n", IS_ERR (onewire_pin));
            return -1 * IS_ERR (onewire_pin);
        }

    // ret = gpiod_get_value(onewire_pin);

    // initializing the character device
    s_dev = kmalloc (sizeof (struct onewire_dev), GFP_KERNEL);
    if (!s_dev)
        {
            ret = -ENOMEM;
            printk (KERN_ERR "%s: Failed to allocate device structure\n",
                    MODULE_NAME);
            goto unregister_region;
        }
    memset (s_dev, 0, sizeof (struct onewire_dev));

    // Allocate kernel buffer
    s_dev->buffer_size = 512; // Use page size for buffer
    s_dev->kernel_buffer = kmalloc (s_dev->buffer_size, GFP_KERNEL);
    if (!s_dev->kernel_buffer)
        {
            ret = -ENOMEM;
            printk (KERN_ERR "%s: Failed to allocate kernel buffer\n",
                    MODULE_NAME);
            goto free_device_struct;
        }
    memset (s_dev->kernel_buffer, 0, sizeof (s_dev->buffer_size));

    // initializing character device
    major_number = register_chrdev (0, MODULE_NAME, &fops);
    if (major_number < 0)
        {
            pr_alert ("Registering char device failed with %d\n",
                      major_number);
            goto free_kernel_buffer;
        }

    // pr_info("I was assigned major number %d.\n", major_number);

    cls = class_create (MODULE_NAME);

    device_create (cls, NULL, MKDEV (major_number, 0), NULL, MODULE_NAME);

    pr_info ("Device created on /dev/%s\n", MODULE_NAME);

    return 0;

free_kernel_buffer:
    kfree (s_dev->kernel_buffer);
free_device_struct:
    kfree (s_dev);
unregister_region:
    unregister_chrdev_region (dev_num, 1);
    return ret;
}

// 21
static int
onewire_remove (struct platform_device *pdev)
{
    pr_info ("onewire:  onewire_remove");

    // destroy character device
    device_destroy (cls, MKDEV (major_number, 0));
    class_destroy (cls);

    // unregister
    unregister_chrdev (major_number, MODULE_NAME);

    // free gpio pin
    gpiod_put (onewire_pin);
    pr_info ("good bye reader!\n");

    return 0;
}

static const struct of_device_id my_of_match[]
    = { { .compatible = "my-onewire" },
        { /* sentinel */ } };
MODULE_DEVICE_TABLE (of, my_of_match);

struct platform_driver my_driver = {
    .driver = {
        .name = MODULE_NAME,
        .of_match_table = my_of_match,
    },
    .probe = onewire_probe,
    .remove = onewire_remove,
};

module_platform_driver (my_driver);

MODULE_LICENSE ("GPL");
// MODULE_LICENSE("Proprietary");