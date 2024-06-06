#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>

#define GPIO_BASE_ADDRESS 0xFF203000
#define GPIO_SIZE 4

int main() {
    int mem_fd;
    void *gpio_map;

    // Open /dev/mem to access physical memory
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        perror("Failed to open /dev/mem");
        exit(EXIT_FAILURE);
    }

    // Map the GPIO register to virtual memory
    gpio_map = mmap(NULL, GPIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE_ADDRESS);
    if (gpio_map == MAP_FAILED) {
        perror("Failed to mmap GPIO register");
        exit(EXIT_FAILURE);
    }

    // Access the GPIO register using the virtual address
    *(volatile unsigned int *)(gpio_map + 1) = 1;

    return 0;
}