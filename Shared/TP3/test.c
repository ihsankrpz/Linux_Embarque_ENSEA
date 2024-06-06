
#include <linux/timer.h>
#include <linux/proc_fs.h>

// Global variables for chenillard
static struct timer_list chenillard_timer;
static unsigned int chenillard_speed = 1000; // Default speed is 1000ms
static unsigned int chenillard_pattern = 0xFF; // Default pattern is all LEDs on
static int chenillard_direction = 1; // Default direction is forward

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
    .read = seq_read,
    .write = chenillard_speed_proc_write,
    .llseek = seq_lseek,
    .release = single_release,
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
    .read = seq_read,
    .write = chenillard_direction_proc_write,
    .llseek = seq_lseek,
    .release = single_release,
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
    
    // Save the device pointer in the timer for chenillard
    dev->chenillard_timer = chenillard_timer;
    
    pr_info("leds_probe exit\n");
    return 0;

bad_ioremap:
   ret_val = PTR_ERR(dev->regs);
bad_exit_return:
    pr_info("leds_probe bad exit :(\n");
    return ret_val;
}

// Gets called whenever a device this driver handles is removed.
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