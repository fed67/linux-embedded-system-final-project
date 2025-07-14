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

#include <linux/kfifo.h>
#include <linux/ktime.h>

#define MAX_SIZE 128

#define MODULE_NAME "onewire_dev"

#define PIN_ONEWIRE_OUT "onewire"
#define PIN_ONEWIRE_IN "onewirein"

#define RESULT_FIFO_SIZE 128

// Structure to hold device-specific data
struct onewire_dev
{
    char *kernel_buffer;
    int buffer_size;
    struct cdev cdev;
};

struct gpio_desc *onewire_pin;
struct gpio_desc *onewire_pin_in;
static struct onewire_dev *s_dev = NULL;
static int major_number = 0;
static dev_t dev_num;

static struct class *cls;

// Data structures
struct read_data_t
{
    char data[8];
    size_t size;
};

static DECLARE_KFIFO (result_fifo, struct read_data_t *, RESULT_FIFO_SIZE);

const char bit_mask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

static int
string_cmp (const char *s1, const char *s2, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (s1[i] != s2[i])
        {
            return 0;
        }
    }
    return 1;
}

/*
static uint8_t compute_crc(char* data, size_t length) {

        uint8_t result = 0x00;

        //char mask = 0x30;
        // 0x98 0x9800
        uint8_t mask = 0x31;

        for(int k = 0; k < length; k++) { //length
                uint8_t byte = data[k];
                //byte = 0x10;

        printk("byte %x\n", byte);
        //result ^= byte;
        for( int i = 0; i < 8; i++) {
                uint8_t mix = ( result ^= byte ) ^ 0x1;
                result >>= 1;
                if( mix ) {
                        result = ( result ^ 0x8C );
                        printk("skip bit i %i result %x \n", i, result);

                }
                byte >>= 1;

        }
        printk("CRC %x result %x data %x \n", result, result, data[k]);
        }

        return result;
}
    */

// Define onewire functions
static int
wait_until_rising_edge (struct gpio_desc *request_in)
{
    int found = -1;
    int value = 0;
    for (int i = 0; i < 10000; i++)
    {
        value = gpiod_get_value (request_in);
        if (value == 1)
        {
            found = i;
            break;
        }
        ndelay (500);
    }
    return found;
}

static int
write_cmd (struct gpio_desc *request_out, char *data, size_t length)
{

    for (int i = 0; i < length; i++)
    {

        for (int j = 0; j < 8; j++)
        {
            if (data[i] & bit_mask[j])
            {
                printk ("Write 1 \n");
                gpiod_set_value (request_out, 0);
                // usleep(1);
                udelay (3);

                gpiod_set_value (request_out, 1);
                udelay (60);
            }
            else
            {
                printk ("Write 0 \n");
                gpiod_set_value (request_out, 0);
                udelay (60);
                gpiod_set_value (request_out, 1);
                udelay (15);
            }
        }
        udelay (30);
    }

    return 0;
}

int
read_cmd (struct gpio_desc *request_out, struct gpio_desc *request_in, char *data, size_t length)
{

    u64 start, end;
    for (int i = 0; i < length; i++)
    {
        char read_bits = 0;
        for (int j = 0; j < 8; j++)
        {
            gpiod_set_value (request_out, 0);
            start = ktime_get_ns ();

            udelay (5);

            gpiod_set_value (request_out, 1);

            wait_until_rising_edge (request_in);

            end = ktime_get_ns ();
            printk ("ret %llu \n", end - start);

            if ((end - start) > 20000)
            {
                printk ("Read 0 \n");
                read_bits = read_bits >> 1;
            }
            else
            {
                printk ("Read 1 \n");
                read_bits = (read_bits >> 1) | 0x80; // put a '1' at bit 7
            }

            udelay (60);
        }
        data[i] = read_bits;
        printk ("------ readd ata %x  -------------\n", read_bits);
    }

    return 0;
}

static void
reset (struct gpio_desc *request_out, struct gpio_desc *request_in)
{
    printk ("run reset \n");

    gpiod_set_value (request_out, 0);

    gpiod_set_value (request_out, 1);

    // wit until device finished pull down
    wait_until_rising_edge (request_in);
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
    printk ("READ \n");
    struct read_data_t *result;

    printk ("size %i \n", kfifo_size (&result_fifo));

    printk ("use kfifo_put \n");
    int ret = kfifo_get (&result_fifo, &result);
    if (ret == 0)
    { // if fifo is empty stop reading
        return 0;
    }
    printk ("ptr_adr %p ret %i \n", result, ret);
    printk ("result size %u \n", result->size);

    // int ret = 	kfifo_get(&result_fifo, result);
    // printk("ptr_adr %p ret %i \n", result, ret);

    // printk("fifo ret %i size %lu \n", ret, result->size);
    if (kfifo_size (&result_fifo))
    {

        // kfifo_get(&result_fifo, &data);

        if (copy_to_user (buf, s_dev->kernel_buffer + *f_pos, count))
        {
            return -EFAULT; // Failed to copy to user space
        }
    }
    else
    {
        if (copy_to_user (buf, s_dev->kernel_buffer + *f_pos, count))
        {
            return -EFAULT; // Failed to copy to user space
        }
    }

    kfree (result);

    *f_pos += count;
    bytes_read = count;
    printk (KERN_INFO "%s: Read %zu bytes from device (offset: %lld)\n", MODULE_NAME, count,
            *f_pos);
    return bytes_read;
}

static ssize_t
onewire_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    // struct onewire_dev *dev = (struct onewire_dev *)filp->private_data;
    ssize_t bytes_written = 0;

    pr_info ("copy count %u f_pos %llu \n", count, *f_pos);
    // pr_info ("dev->buffer_size %lu \n", s_dev->buffer_size);

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

    if (count > 0)
    {
        if (s_dev->kernel_buffer[0] == 'r')
        {
            reset (onewire_pin, onewire_pin_in);
        }
        else if (string_cmp (s_dev->kernel_buffer, "ReadAddress", 11))
        {
            reset (onewire_pin, onewire_pin_in);
            int ret = wait_until_rising_edge (onewire_pin_in);
            printk ("ret wait %i \n", ret);
            udelay (300);

            // char data[2] = { 0xCC, 0xBE };
            char data[1] = { 0x33 };
            write_cmd (onewire_pin, data, 1);

            udelay (600);
            char data_read[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            read_cmd (onewire_pin, onewire_pin_in, data_read, 8);

            struct read_data_t *result = kmalloc (sizeof (struct read_data_t), GFP_KERNEL);
            for (int i = 0; i < 8; i++)
            {
                result->data[i] = data_read[i];
            }
            result->size = 8;

            printk ("prt adr %p &adr %p \n", result, &result);
            kfifo_put (&result_fifo, result);
        }
        else
        {

            pr_info ("value");
            int value = s_dev->kernel_buffer[0] - '0';
            pr_info ("value %i\n", value);
            gpiod_set_value (onewire_pin, value);
        }
    }

    printk (KERN_INFO "%s: Wrote %zu bytes to device (offset: %lld)\n", MODULE_NAME, count, *f_pos);
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

    // checking if the device haas the property label
    if (!device_property_present (dev, "label"))
    {
        pr_crit ("Device property 'label' not found!\n");
        return -1;
    }

    // checking if the device haas the property onewire-gpio
    if (!device_property_present (dev, "onewire-gpio"))
    {
        pr_crit ("Device property 'onewire-gpio' not found!\n");
        return -1;
    }

    // reading the device property lable in the device tree
    int ret = device_property_read_string (dev, "label", &label);
    if (ret)
    {
        pr_crit ("dt_gpio - Error! Could not read 'label'\n");
        return -1;
    }
    pr_info ("dt_gpio - label: %s\n", label);

    onewire_pin = gpiod_get (dev, PIN_ONEWIRE_OUT, GPIOD_OUT_HIGH);
    if (IS_ERR (onewire_pin))
    {
        pr_crit ("gpiod_get error %i\n", onewire_pin);
        return -1 * IS_ERR (onewire_pin);
    }

    printk ("used gpio 2 \n");

    onewire_pin_in = gpiod_get (dev, PIN_ONEWIRE_IN, GPIOD_OUT_HIGH);
    if (IS_ERR (onewire_pin_in))
    {
        pr_crit ("gpiod_get error \n");
        return -1 * IS_ERR (onewire_pin_in);
    }

    gpiod_direction_input (onewire_pin_in);
    gpiod_direction_output (onewire_pin, GPIOD_OUT_HIGH);

    // initializing the character device
    s_dev = kmalloc (sizeof (struct onewire_dev), GFP_KERNEL);
    if (!s_dev)
    {
        ret = -ENOMEM;
        printk (KERN_ERR "%s: Failed to allocate device structure\n", MODULE_NAME);
        goto unregister_region;
    }
    memset (s_dev, 0, sizeof (struct onewire_dev));

    // Allocate kernel buffer
    s_dev->buffer_size = 512; // Use page size for buffer
    s_dev->kernel_buffer = kmalloc (s_dev->buffer_size, GFP_KERNEL);
    if (!s_dev->kernel_buffer)
    {
        ret = -ENOMEM;
        printk (KERN_ERR "%s: Failed to allocate kernel buffer\n", MODULE_NAME);
        goto free_device_struct;
    }
    memset (s_dev->kernel_buffer, 0, sizeof (s_dev->buffer_size));

    // initializing character device
    major_number = register_chrdev (0, MODULE_NAME, &fops);
    if (major_number < 0)
    {
        pr_alert ("Registering char device failed with %d\n", major_number);
        goto free_kernel_buffer;
    }

    cls = class_create (MODULE_NAME);

    device_create (cls, NULL, MKDEV (major_number, 0), NULL, MODULE_NAME);

    pr_info ("Device created on /dev/%s\n", MODULE_NAME);

    // initialize data structures
    kfifo_alloc (&result_fifo, RESULT_FIFO_SIZE, GFP_KERNEL);

    return 0;

free_kernel_buffer:
    kfree (s_dev->kernel_buffer);
free_device_struct:
    kfree (s_dev);
unregister_region:
    unregister_chrdev_region (dev_num, 1);
    return ret;
}

static int
onewire_remove (struct platform_device *pdev)
{
    pr_info ("onewire:  onewire_remove");

    kfree (s_dev->kernel_buffer);

    kfifo_free (&result_fifo);

    // free gpio pins
    gpiod_put (onewire_pin);
    gpiod_put (onewire_pin_in);

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
    = { { .compatible = "my-onewire" }, { /* sentinel */ } };
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