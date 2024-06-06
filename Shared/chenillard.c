#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("%d\n", getpid());

    while (1)
    {
        // ONE WAY
        // for (int X = 1; X <= 9; X++) {
        //     char command[100];
        //     sprintf(command, "echo \"1\" > /sys/class/leds/fpga_led%d/brightness", X);
        //     system(command);
        //     usleep(100000); // Delay of 100ms
        //     sprintf(command, "echo \"0\" > /sys/class/leds/fpga_led%d/brightness", X);
        //     system(command);
        // }
        // for (int X = 9; X >= 1; X--) {
        //     char command[100];
        //     sprintf(command, "echo \"1\" > /sys/class/leds/fpga_led%d/brightness", X);
        //     system(command);
        //     usleep(100000); // Delay of 100ms
        //     sprintf(command, "echo \"0\" > /sys/class/leds/fpga_led%d/brightness", X);
        //     system(command);
        // }

        //ANTOHER WAY
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
    }

    return 0;
}





