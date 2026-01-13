#include <stdint.h>

volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;

void putchar(char c, uint8_t color, int x, int y) {
    vga_buffer[y * 80 + x] = (color << 8) | c;
}

void print(const char* str, uint8_t color) {
    int x = 0, y = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            y++;
            x = 0;
        } else {
            putchar(str[i], color, x, y);
            x++;
        }
    }
}

void kernel_main(void) {
    // Clear screen
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (0x00 << 8) | ' ';
    }
    
    print("MyOS i686 v0.1\n", 0x0F);
    print("Kernel loaded successfully!", 0x0A);
    
    while(1) {
        __asm__("hlt");
    }
}
