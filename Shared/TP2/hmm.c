#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>

#define PROC_ENTRY_FILENAME "my_proc_entry"

static int my_proc_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data)
{
    int ret;
    ret = snprintf(buffer, buffer_length, "Hello from /proc!\n");
    return ret;
}

static int __init hello_init(void)
{
    struct proc_dir_entry *proc_entry;
    
    proc_entry = proc_create(PROC_ENTRY_FILENAME, 0, NULL, &my_proc_fops);
    if (!proc_entry) {
        printk(KERN_ERR "Failed to create /proc entry\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "Hello, module loaded!\n");
    return 0;
}

static void __exit hello_exit(void)
{
    remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
    printk(KERN_INFO "Goodbye, module unloaded!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple hello world module");