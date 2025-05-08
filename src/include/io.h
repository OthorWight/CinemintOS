#ifndef IO_H
#define IO_H

#include <stdint.h>
#include "vectors.h" // For vector<char> used in input() and print_vector()
#include "consts.h"  // For VGA_COLOR_*, VGA_WIDTH, VGA_HEIGHT, KEY_LIMIT, etc.

// --- Extern Global Variable Declarations (these are defined in screen.cpp or consts.cpp) ---
// These remind us that the I/O functions rely on these globals being defined elsewhere.
extern volatile uint16_t* vga_buffer; // Defined in screen.cpp
extern uint16_t cursor_x;             // Defined in screen.cpp
extern uint16_t cursor_y;             // Defined in screen.cpp

// extern const uint16_t VGA_WIDTH; // Defined in consts.cpp (via consts.h)
// extern const uint16_t VGA_HEIGHT; // Defined in consts.cpp (via consts.h)
// extern const char scancode_ascii_normal[]; // Defined in consts.cpp (via consts.h)
// extern const char scancode_ascii_shifted[]; // Defined in consts.cpp (via consts.h)
// extern const uint8_t KEY_LIMIT; // Defined in consts.cpp (via consts.h)
// Key scancode #defines (like ENTER, BACKSPACE) are directly used from consts.h

// --- Forward declaration for scroll_screen (defined in screen.cpp) ---
void scroll_screen(volatile uint16_t* buffer); // Or void scroll_screen();


// --- I/O Port Functions (static inline, so definition stays in header) ---
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ( "outl %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ( "inl %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// --- Character and String Printing Function Declarations ---
void print_char(char c, bool inplace = false, int color = VGA_COLOR_LIGHT_GREY);
void print_string(const char* str, int color = VGA_COLOR_LIGHT_GREY);
void print_vector(const vector<char>& v, int color = VGA_COLOR_LIGHT_GREY);

void print_uint_base(unsigned long long n, int base, int color, bool print_prefix);
void print_int(long long n, int color = VGA_COLOR_LIGHT_GREY);
void print_hex(uint64_t n, int color = VGA_COLOR_LIGHT_GREY);
void print_hex32(uint32_t n, int color = VGA_COLOR_LIGHT_GREY);

// --- Keyboard Input Function Declarations ---
char scancode_to_ascii(uint8_t scancode, bool shift_pressed);
uint8_t scankey(); // Blocking call to get a scancode
void input(vector<char>& v, int color = VGA_COLOR_LIGHT_GREY);

#endif // IO_H