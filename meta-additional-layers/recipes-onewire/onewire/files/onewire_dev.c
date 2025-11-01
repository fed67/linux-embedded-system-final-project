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


static const uint8_t onewire_crc8_table[256] = {
    #define P(x) x
    P(0x00),0x5E,0xBC,0xE2,0x61,0x3F,0xDD,0x83,0xC2,0x9C,0x7E,0x20,0xA3,0xFD,0x1F,0x41,
    0x9D,0xC3,0x21,0x7F,0xFC,0xA2,0x40,0x1E,0x5F,0x01,0xE3,0xBD,0x3E,0x60,0x82,0xDC,
    0x23,0x7D,0x9F,0xC1,0x42,0x1C,0xFE,0xA0,0xE1,0xBF,0x5D,0x03,0x80,0xDE,0x3C,0x62,
    0xBE,0xE0,0x02,0x5C,0xDF,0x81,0x63,0x3D,0x7C,0x22,0xC0,0x9E,0x1D,0x43,0xA1,0xFF,
    0x46,0x18,0xFA,0xA4,0x27,0x79,0x9B,0xC5,0x84,0xDA,0x38,0x66,0xE5,0xBB,0x59,0x07,
    0xDB,0x85,0x67,0x39,0xBA,0xE4,0x06,0x58,0x19,0x47,0xA5,0xFB,0x78,0x26,0xC4,0x9A,
    0x65,0x3B,0xD9,0x87,0x04,0x5A,0xB8,0xE6,0xA7,0xF9,0x1B,0x45,0xC6,0x98,0x7A,0x24,
    0xF8,0xA6,0x44,0x1A,0x99,0xC7,0x25,0x7B,0x3A,0x64,0x86,0xD8,0x5B,0x05,0xE7,0xB9,
    0x8C,0xD2,0x30,0x6E,0xED,0xB3,0x51,0x0F,0x4E,0x10,0xF2,0xAC,0x2F,0x71,0x93,0xCD,
    0x11,0x4F,0xAD,0xF3,0x70,0x2E,0xCC,0x92,0xD3,0x8D,0x6F,0x31,0xB2,0xEC,0x0E,0x50,
    0xAF,0xF1,0x13,0x4D,0xCE,0x90,0x72,0x2C,0x6D,0x33,0xD1,0x8F,0x0C,0x52,0xB0,0xEE,
    0x32,0x6C,0x8E,0xD0,0x53,0x0D,0xEF,0xB1,0xF0,0xAE,0x4C,0x12,0x91,0xCF,0x2D,0x73,
    0xCA,0x94,0x76,0x28,0xAB,0xF5,0x17,0x49,0x08,0x56,0xB4,0xEA,0x69,0x37,0xD5,0x8B,
    0x57,0x09,0xEB,0xB5,0x36,0x68,0x8A,0xD4,0x95,0xCB,0x29,0x77,0xF4,0xAA,0x48,0x16,
    0xE9,0xB7,0x55,0x0B,0x88,0xD6,0x34,0x6A,0x2B,0x75,0x97,0xC9,0x4A,0x14,0xF6,0xA8,
    0x74,0x2A,0xC8,0x96,0x15,0x4B,0xA9,0xF7,0xB6,0xE8,0x0A,0x54,0xD7,0x89,0x6B,0x35
};

static uint8_t compute_crc(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    while (len--) {
        crc = onewire_crc8_table[crc ^ *data++];
    }
    return crc;
}


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
    printk ("Write CMD \n");
    printk ("Write length %u \n", length);
    // gpiod_direction_output(request_out, 1);

    gpiod_direction_input(request_out);
    for (int i = 0; i < length; i++)
    {

        for (int j = 0; j < 8; j++)
        {
            
            unsigned long flags;
            local_irq_save(flags);

            if (data[i] & bit_mask[j])
            {
                printk ("Write 1 \n");
                // gpiod_set_value (request_out, 0);
                gpiod_direction_output(request_out, 0);
                // usleep(1);
                // udelay (3);
                udelay (7);

                // gpiod_set_value (request_out, 1);
                gpiod_direction_input(request_out);
                local_irq_restore(flags);
                udelay (60);
            }
            else
            {
                printk ("Write 0 \n");
                // gpiod_set_value (request_out, 0);
                gpiod_direction_output(request_out, 0);
                udelay (60);
                // gpiod_set_value (request_out, 1);
                gpiod_direction_input(request_out);
                local_irq_restore(flags);

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
            unsigned long flags;
            local_irq_save(flags);

            gpiod_direction_output(request_out, 0);
            //gpiod_set_value (request_out, 0);
            start = ktime_get_ns ();

            udelay (9);

            //gpiod_set_value (request_out, 1);

            gpiod_direction_input(request_out);
            wait_until_rising_edge (request_out);
            // wait_until_rising_edge (request_in);

            // udelay(10);
            // int rd = gpiod_get_value(request_out);

            // printk("rd %u ", rd);
            // if (rd == 0)
            // {
            //     printk ("Read 0 \n");
            //     read_bits = read_bits >> 1;
            // }
            // else
            // {
            //     printk ("Read 1 \n");
            //     read_bits = (read_bits >> 1) | 0x80; // put a '1' at bit 7
            // }
            

            end = ktime_get_ns ();
            local_irq_restore(flags);

            printk ("measured time %llu \n", end - start);

            if ((end - start) > 35000) // measured by oscilloscope
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
        printk ("------ readd data %x  -------------\n", read_bits);
    }

    gpiod_direction_input(request_out);


    for(int i = 0; i < length; i++) {
        printk("%x ", data[i]);
    }
    uint8_t crc = compute_crc(data, length-1);
    printk("crc %x ", crc);
    printk("length %i ", length);
    return 0;
}

static void
reset (struct gpio_desc *request_out, struct gpio_desc *request_in)
{
    printk ("run reset \n");

    gpiod_direction_output(request_out, 1);
    gpiod_set_value (request_out, 0);

    udelay(500);

    gpiod_set_value (request_out, 1);

    gpiod_direction_input(request_out);
    // wit until device finished pull down
    //wait_until_rising_edge (request_in);


    // int ret = wait_until_rising_edge (onewire_pin_in);
    int ret = wait_until_rising_edge (request_out);
    printk ("ret wait %i \n", ret);

    udelay (500);
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
    printk("f_pos %lu count %lu \n", f_pos, count); 
    struct read_data_t *result;

    printk ("size %i \n", kfifo_size (&result_fifo));

    printk ("use kfifo_put \n");
    int processed_elements = kfifo_get (&result_fifo, &result);
    printk ("ptr_adr %p processed_elements %i \n", result, processed_elements);
    if (processed_elements == 0)
    { // if fifo is empty stop reading
        return 0;
    }

    printk ("result size %lu \n", result->size);

    // int ret = 	kfifo_get(&result_fifo, result);
    // printk("ptr_adr %p ret %i \n", result, ret);

    // printk("fifo ret %i size %lu \n", ret, result->size);
    //if (kfifo_size (&result_fifo))
    //{

        //int processed_elements = kfifo_get(&result_fifo, &result);

	for(size_t i = 0; i < result->size; i++) {
		printk("c %x ", result->data[i]);
		s_dev->kernel_buffer[i] = result->data[i];
	}


	size_t min_count = s_dev->buffer_size;
	if(  result->size < min_count )
		min_count = result->size;

	printk("result size %lu min_count %u \n", result->size, min_count);
        if (copy_to_user (buf, s_dev->kernel_buffer, min_count))
        {
            return -EFAULT; // Failed to copy to user space
        }
    /*}
    else
    {
        if (copy_to_user (buf, s_dev->kernel_buffer + *f_pos, count))
        {
            return -EFAULT; // Failed to copy to user space
        }
    } */

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
        else if (s_dev->kernel_buffer[0] == 'h')
        {
            gpiod_direction_output(onewire_pin, 1);
            gpiod_set_value (onewire_pin, 1);

        }
        else if (s_dev->kernel_buffer[0] == 'l')
        {
            gpiod_direction_output(onewire_pin, 0);
            gpiod_set_value (onewire_pin, 0);

        }
        else if (s_dev->kernel_buffer[0] == 'i')
        {
            gpiod_direction_input(onewire_pin);
        }
        else if (string_cmp (s_dev->kernel_buffer, "22", 2)) // Read Address
        {
	        printk("Read Address \n");
            reset (onewire_pin, onewire_pin_in);

	
            // char data[2] = { 0xCC, 0xBE };
            char data[1] = { 0x22 };
            write_cmd (onewire_pin, data, 1);
        } else if (string_cmp (s_dev->kernel_buffer, "CC", 2)) // Read Address
        {
	        printk("Read Address \n");
            reset (onewire_pin, onewire_pin_in);

	
            // char data[2] = { 0xCC, 0xBE };
            char data[1] = { 0xCC };
            write_cmd (onewire_pin, data, 1);
        }
        else if (string_cmp (s_dev->kernel_buffer, "RA", 2)) // Read Address
        {
	        printk("Read Address \n");
            reset (onewire_pin, onewire_pin_in);

	
            // char data[2] = { 0xCC, 0xBE };
            char data[1] = { 0x33 };
            write_cmd (onewire_pin, data, 1);

            udelay (500);
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
        else if (string_cmp (s_dev->kernel_buffer, "WS", 2)) // Write Scrathpad
        {
            reset (onewire_pin, onewire_pin_in);


            char data[7];
	    data[0] = 0x4E;
            data[1] = 0x4E;
            for (int i = 0; i < min (5, count - 2); i++)
            {
                data[i + 2] = s_dev->kernel_buffer[i + 3];
            }
            write_cmd (onewire_pin, data, sizeof(data));

            udelay (600);
        }
        else if (string_cmp (s_dev->kernel_buffer, "RS", 2)) // Read Scrathpad
        {
	        printk("Read scratchpad \n");
            reset (onewire_pin, onewire_pin_in);


            char data[2];
            data[0] = 0xCC;
	        data[1] = 0xBE;
            write_cmd (onewire_pin, data, 2);

            udelay (600);

            char data_read[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0, 0x0 };
            read_cmd (onewire_pin, onewire_pin_in, data_read, 8+1);

            struct read_data_t *result = kmalloc (sizeof (struct read_data_t), GFP_KERNEL);
            for (int i = 0; i < 8; i++)
            {
                result->data[i] = data_read[i];
            }
            result->size = 8;

            printk ("prt adr %p &adr %p \n", result, &result);
            kfifo_put (&result_fifo, result);
        }
        else if (string_cmp (s_dev->kernel_buffer, "CT", 2)) // convert temperature
        {
            reset (onewire_pin, onewire_pin_in);


            char data[2];
	        data[0] = 0xCC;
            data[1] = 0x44;
            write_cmd (onewire_pin, data, 2);

            udelay (200);
        }
        else if (string_cmp (s_dev->kernel_buffer, "CP", 2)) // copy temp
        {
            reset (onewire_pin, onewire_pin_in);


            char data[2];
	        data[0] = 0xCC;
            data[1] = 0x48;
            write_cmd (onewire_pin, data, 2);

            udelay (200);
        }  else  // Read Address
        {
	        printk("Read Address \n");
            reset (onewire_pin, onewire_pin_in);

	
            // char data[2] = { 0xCC, 0xBE };
            char data[1] = { s_dev->kernel_buffer[0] };
            write_cmd (onewire_pin, data, 1);
        }
        // else
        // {

        //     pr_info ("value");
        //     int value = s_dev->kernel_buffer[0] - '0';
        //     pr_info ("value %i\n", value);
        //     gpiod_set_value (onewire_pin, value);
        // }
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
