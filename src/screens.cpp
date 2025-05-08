#include "include/screens.h"
#include "include/consts.h" // For VGA_WIDTH, VGA_HEIGHT, VGA_DEFAULT_COLOR

// --- Screen Global Variable Definitions ---
volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
uint16_t cursor_x = 0;
uint16_t cursor_y = 0;

// --- Screen Function Definitions ---

void cls() {
    for (uint16_t y = 0; y < VGA_HEIGHT; ++y) {
        for (uint16_t x = 0; x < VGA_WIDTH; ++x) {
            // Use VGA_DEFAULT_COLOR from consts.h for the background/foreground
            vga_buffer[y * VGA_WIDTH + x] = (VGA_DEFAULT_COLOR << 8) | ' ';
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void scroll_screen(volatile uint16_t* buffer_to_scroll) {
    // If you chose the void scroll_screen() version in the header,
    // remove the parameter and use the global vga_buffer directly here:
    // volatile uint16_t* buffer_to_scroll = vga_buffer;

    for (uint16_t y = 0; y < VGA_HEIGHT - 1; ++y) {
        for (uint16_t x = 0; x < VGA_WIDTH; ++x) {
            buffer_to_scroll[y * VGA_WIDTH + x] = buffer_to_scroll[(y + 1) * VGA_WIDTH + x];
        }
    }
    // Clear the last line
    for (uint16_t x = 0; x < VGA_WIDTH; ++x) {
        // Use VGA_DEFAULT_COLOR for the new empty line's attributes
        buffer_to_scroll[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (VGA_DEFAULT_COLOR << 8) | ' ';
    }
    // Note: The cursor_y is typically adjusted by the print_char function
    // when it detects it has gone past VGA_HEIGHT - 1 *after* scrolling.
    // So, scroll_screen itself might not need to touch cursor_y directly,
    // but print_char will set it to VGA_HEIGHT - 1.
}