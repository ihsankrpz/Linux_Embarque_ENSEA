#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>

// Prototypes
static int leds_probe(struct platform_device *pdev);
static int leds_remove(struct platform_device *pdev);
static ssize_t leds_read(struct file *file, char *buffer, size_t len, loff_t *offset);
static ssize_t leds_write(struct file *file, const char *buffer, size_t len, loff_t *offset);

// Global variables for chenillard
static struct timer_list chenillard_timer;
static unsigned int chenillard_speed = 1000; // Default speed is 1000ms
static unsigned int chenillard_pattern = 0xFF; // Default pattern is all LEDs on
static int chenillard_direction = 1; // Default direction is forward

module_param(chenillard_speed, int, 0);
MODULE_PARM_DESC(chenillard_speed, "chenillard_speed");

module_param(chenillard_direction, int, 0);
MODULE_PARM_DESC(chenillard_direction, "chenillard_direction");

// An instance of this structure will be created for every ensea_led IP in the system
struct ensea_leds_dev {
    struct miscdevice miscdev;
    void __iomem *regs;
    u8 leds_value;
};

// Specify which device tree devices this driver supports
static struct of_device_id ensea_leds_dt_ids[] = {
    {
        .compatible = "dev,ensea"
    },
    { /* end of table */ }
};

// Inform the kernel about the devices this driver supports
MODULE_DEVICE_TABLE(of, ensea_leds_dt_ids);

// Data structure that links the probe and remove functions with our driver
static struct platform_driver leds_platform = {
    .probe = leds_probe,
    .remove = leds_remove,
    .driver = {
        .name = "Ensea LEDs Driver",
        .owner = THIS_MODULE,
        .of_match_table = ensea_leds_dt_ids
    }
};

// The file operations that can be performed on the ensea_leds character file
static const struct file_operations ensea_leds_fops = {
    .owner = THIS_MODULE,
    .read = leds_read,
    .write = leds_write
};

// Called when the driver is installed
static int leds_init(void)
{
    int ret_val = 0;
    pr_info("Initializing the Ensea LEDs module\n");

    // Register our driver with the "Platform Driver" bus
    ret_val = platform_driver_register(&leds_platform);
    if(ret_val != 0) {
        pr_err("platform_driver_register returned %d\n", ret_val);
        return ret_val;
    }

    // Initialize the chenillard timer
    setup_timer(&chenillard_timer, chenillard_timer_callback, 0);
    mod_timer(&chenillard_timer, jiffies + msecs_to_jiffies(chenillard_speed));
    
    // Create /proc/ensea/speed file for reading and writing chenillard speed
    proc_create("ensea/speed", 0666, NULL, &chenillard_speed_proc_fops);
    
    // Create /proc/ensea/dir file for reading and writing chenillard direction
    proc_create("ensea/dir", 0666, NULL, &chenillard_direction_proc_fops);

    pr_info("Ensea LEDs module successfully initialized!\n");

    return 0;
}

// Called whenever the kernel finds a new device that our driver can handle
// (In our case, this should only get called for the one instantiation of the Ensea LEDs module)
static int leds_probe(struct platform_device *pdev)
{
    int ret_val = -EBUSY;
    struct ensea_leds_dev *dev;
    struct resource *r = 0;

    pr_info("leds_probe enter\n");

    // Get the memory resources for this LED device
    r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(r == NULL) {
        pr_err("IORESOURCE_MEM (register space) does not exist\n");
        goto bad_exit_return;
    }

    // Create structure to hold device-specific information (like the registers)
    dev = devm_kzalloc(&pdev->dev, sizeof(struct ensea_leds_dev), GFP_KERNEL);

    // Both request and ioremap a memory region
    // This makes sure nobody else can grab this memory region
    // as well as moving it into our address space so we can actually use it
    dev->regs = devm_ioremap_resource(&pdev->dev, r);
    if(IS_ERR(dev->regs))
        goto bad_ioremap;

    // Turn the LEDs on (access the 0th register in the ensea LEDs module)
    dev->leds_value = 0xFF;
    iowrite32(dev->leds_value, dev->regs);

    // Initialize the misc device (this is used to create a character file in userspace)
    dev->miscdev.minor = MISC_DYNAMIC_MINOR;    // Dynamically choose a minor number
    dev->miscdev.name = "ensea_leds";
    dev->miscdev.fops = &ensea_leds_fops;

    ret_val = misc_register(&dev->miscdev);
    if(ret_val != 0) {
        pr_info("Couldn't register misc device :(");
        goto bad_exit_return;
    }

    // Give a pointer to the instance-specific data to the generic platform_device structure
    // so we can access this data later on (for instance, in the read and write functions)
    platform_set_drvdata(pdev, (void*)dev);

    pr_info("leds_probe exit\n");

    return 0;

bad_ioremap:
   ret_val = PTR_ERR(dev->regs);
bad_exit_return:
    pr_info("leds_probe bad exit :(\n");
    return ret_val;
}

// This function gets called whenever a read operation occurs on one of the character files
static ssize_t leds_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
    int success = 0;

    /*
    * Get the ensea_leds_dev structure out of the miscdevice structure.
    *
    * Remember, the Misc subsystem has a default "open" function that will set
    * "file"s private data to the appropriate miscdevice structure. We then use the
    * container_of macro to get the structure that miscdevice is stored inside of (which
    * is our ensea_leds_dev structure that has the current led value).
    *
    * For more info on how container_of works, check out:
    * http://linuxwell.com/2012/11/10/magical-container_of-macro/
    */
    struct ensea_leds_dev *dev = container_of(file->private_data, struct ensea_leds_dev, miscdev);

    // Give the user the current led value
    success = copy_to_user(buffer, &dev->leds_value, sizeof(dev->leds_value));

    // If we failed to copy the value to userspace, display an error message
    if(success != 0) {
        pr_info("Failed to return current led value to userspace\n");
        return -EFAULT; // Bad address error value. It's likely that "buffer" doesn't point to a good address
    }

    return 0; // "0" indicates End of File, aka, it tells the user process to stop reading
}

// This function gets called whenever a write operation occurs on one of the character files
static ssize_t leds_write(struct file *file, const char *buffer, size_t len, loff_t *offset)
{
    int success = 0;

    /*
    * Get the ensea_leds_dev structure out of the miscdevice structure.
    *
    * Remember, the Misc subsystem has a default "open" function that will set
    * "file"s private data to the appropriate miscdevice structure. We then use the
    * container_of macro to get the structure that miscdevice is stored inside of (which
    * is our ensea_leds_dev structure that has the current led value).
    *
    * For more info on how container_of works, check out:
    * http://linuxwell.com/2012/11/10/magical-container_of-macro/
    */
    struct ensea_leds_dev *dev = container_of(file->private_data, struct ensea_leds_dev, miscdev);

    // Get the new led value (this is just the first byte of the given data)
    success = copy_from_user(&dev->leds_value, buffer, sizeof(dev->leds_value));

    // If we failed to copy the value from userspace, display an error message
    if(success != 0) {
        pr_info("Failed to read led value from userspace\n");
        return -EFAULT; // Bad address error value. It's likely that "buffer" doesn't point to a good address
    } else {
        // We read the data correctly, so update the LEDs
        iowrite32(dev->leds_value, dev->regs);
    }

    // Tell the user process that we wrote every byte they sent
    // (even if we only wrote the first value, this will ensure they don't try to re-write their data)
    return len;
}

// Gets called whenever a device this driver handles is removed.
// This will also get called for each device being handled when
// our driver gets removed from the system (using the rmmod command).
static int leds_remove(struct platform_device *pdev)
{
    // Grab the instance-specific information out of the platform device
    struct ensea_leds_dev *dev = (struct ensea_leds_dev*)platform_get_drvdata(pdev);

    pr_info("leds_remove enter\n");

    // Turn the LEDs off
    iowrite32(0x00, dev->regs);

    // Unregister the character file (remove it from /dev)
    misc_deregister(&dev->miscdev);

    pr_info("leds_remove exit\n");

    return 0;
}

// Called when the driver is removed
static void leds_exit(void)
{
    pr_info("Ensea LEDs module exit\n");
    
    // Unregister our driver from the "Platform Driver" bus
    // This will cause "leds_remove" to be called for each connected device
    platform_driver_unregister(&leds_platform);
    
    // Remove /proc/ensea/speed file
    remove_proc_entry("ensea/speed", NULL);
    
    // Remove /proc/ensea/dir file
    remove_proc_entry("ensea/dir", NULL);
    
    // Delete the chenillard timer
    del_timer_sync(&chenillard_timer);
    
    pr_info("Ensea LEDs module successfully unregistered\n");
}


// Timer callback function for chenillard
static void chenillard_timer_callback(struct timer_list *timer)
{
    struct ensea_leds_dev *dev = container_of(timer, struct ensea_leds_dev, chenillard_timer);
    
    // Update the LEDs based on the current pattern
    iowrite32(dev->leds_value, dev->regs);
    
    // Update the pattern based on the current direction
    if (chenillard_direction == 1) {
        chenillard_pattern <<= 1;
        if (chenillard_pattern == 0x00) {
            chenillard_pattern = 0x01;
        }
    } else {
        chenillard_pattern >>= 1;
        if (chenillard_pattern == 0x00) {
            chenillard_pattern = 0x80;
        }
    }
    
    // Set the new pattern value
    dev->leds_value = chenillard_pattern;
    
    // Restart the timer
    mod_timer(&chenillard_timer, jiffies + msecs_to_jiffies(chenillard_speed));
}

// Proc file for reading and writing chenillard speed
static int chenillard_speed_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%u\n", chenillard_speed);
    return 0;
}

static int chenillard_speed_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, chenillard_speed_proc_show, NULL);
}

static ssize_t chenillard_speed_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
    unsigned int new_speed;
    if (kstrtouint_from_user(buffer, count, 10, &new_speed) == 0) {
        chenillard_speed = new_speed;
    }
    return count;
}

static const struct file_operations chenillard_speed_proc_fops = {
    .owner = THIS_MODULE,
    .open = chenillard_speed_proc_open,
    .read = leds_read,
    .write = chenillard_speed_proc_write
};

// Proc file for reading and writing chenillard direction
static int chenillard_direction_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%d\n", chenillard_direction);
    return 0;
}

static int chenillard_direction_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, chenillard_direction_proc_show, NULL);
}

static ssize_t chenillard_direction_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
    int new_direction;
    if (kstrtoint_from_user(buffer, count, 10, &new_direction) == 0) {
        chenillard_direction = new_direction;
    }
    return count;
}

static const struct file_operations chenillard_direction_proc_fops = {
    .owner = THIS_MODULE,
    .open = chenillard_direction_proc_open,
    .read = leds_read,
    .write = chenillard_direction_proc_write
};


// Tell the kernel which functions are the initialization and exit functions
module_init(leds_init);
module_exit(leds_exit);

// Define information about this kernel module
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devon Andrade <devon.andrade@oit.edu>");
MODULE_DESCRIPTION("Exposes a character device to user space that lets users turn LEDs on and off");
MODULE_VERSION("1.0");






