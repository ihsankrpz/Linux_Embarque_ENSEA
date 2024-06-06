#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

MODULE_AUTHOR("Bibi");
MODULE_DESCRIPTION("Chenillard");
MODULE_SUPPORTED_DEVICE("Presque tous");
MODULE_LICENSE("GPL");

static int vitesse;
module_param(vitesse, int, 0);
MODULE_PARM_DESC(vitesse, "vitesse du chenillard");

static int __init le_module_init(void) {
    printk(KERN_INFO "Hello world!\n");
    printk(KERN_DEBUG "le paramètre est=%d\n", vitesse);
    for (int X = 1; X <= 9; X++) {
        char filename[100];
        sprintf(filename, "/sys/class/leds/fpga_led%d/brightness", X);
        FILE *file = fopen(filename, "w");
        if (file != NULL) {
        fwrite("1", sizeof(char), 1, file);
        fclose(file);
        }
        usleep(100000); // Delay of 100ms
        file = fopen(filename, "w");
        if (file != NULL) {
        fwrite("0", sizeof(char), 1, file);
        fclose(file);
        }
    }
    for (int X = 9; X >= 1; X--) {
        char filename[100];
        sprintf(filename, "/sys/class/leds/fpga_led%d/brightness", X);
        FILE *file = fopen(filename, "w");
        if (file != NULL) {
        fwrite("1", sizeof(char), 1, file);
        fclose(file);
        }
        usleep(100000); // Delay of 100ms
        file = fopen(filename, "w");
        if (file != NULL) {
        fwrite("0", sizeof(char), 1, file);
        fclose(file);
        }
    }
    return 0;
}

static void __exit le_module_exit(void) {
    printk(KERN_ALERT "Bye bye...\n");
}

module_init(le_module_init);
module_exit(le_module_exit);

