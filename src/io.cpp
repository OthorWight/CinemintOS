#include "include/io.h"
#include "include/screens.h" // For scroll_screen() and screen globals like vga_buffer, cursor_x/y
#include "include/consts.h"  // For VGA_WIDTH, VGA_HEIGHT, KEY_LIMIT, key scancodes, etc.

// --- Extern Global Variable Definitions (these are actually defined elsewhere, but io.cpp uses them via headers) ---
// No need to redefine them here; they are accessed via their declarations in included headers.
// volatile uint16_t* vga_buffer; // Defined in screen.cpp
// uint16_t cursor_x;             // Defined in screen.cpp
// uint16_t cursor_y;             // Defined in screen.cpp
// const uint16_t VGA_WIDTH;      // Defined in consts.cpp
// const uint16_t VGA_HEIGHT;     // Defined in consts.cpp
// const char scancode_ascii_normal[]; // Defined in consts.cpp
// const char scancode_ascii_shifted[];// Defined in consts.cpp
// const uint8_t KEY_LIMIT;       // Defined in consts.cpp


// --- Character and String Printing Function Definitions ---

void print_char(char c, bool inplace, int color) { // Removed default args as they are in header
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\t') {
        int spaces_to_add = 4 - (cursor_x % 4);
        for (int i = 0; i < spaces_to_add; ++i) {
            print_char(' ', false, color);
        }
        return;
    } else {
        // This check might be slightly redundant if scroll_screen is always called correctly,
        // but it's a safe guard.
        if (cursor_y >= VGA_HEIGHT) {
             scroll_screen(vga_buffer); // Assuming global vga_buffer is intended if param is used
             cursor_y = VGA_HEIGHT - 1;
        }
        
        // Ensure cursor_x is also within bounds after a potential scroll or newline from previous char
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
            // Check cursor_y again if wrapping cursor_x caused it to go out of bounds
            if (cursor_y >= VGA_HEIGHT) {
                 scroll_screen(vga_buffer);
                 cursor_y = VGA_HEIGHT - 1;
            }
        }


        uint16_t position = cursor_y * VGA_WIDTH + cursor_x;
        // Final boundary check before writing to VGA buffer
        if (position < (unsigned int)(VGA_WIDTH * VGA_HEIGHT)) { // Cast to unsigned for comparison if VGA_WIDTH/HEIGHT are large
             vga_buffer[position] = (uint16_t)((color << 8) | c);
        }

        if (!inplace) {
            cursor_x++;
            if (cursor_x >= VGA_WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }
        }
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen(vga_buffer); // Use global vga_buffer, or pass it if design prefers
        cursor_y = VGA_HEIGHT - 1;
    }
}

void print_string(const char* str, int color) { // Default arg in header
    for (int i = 0; str[i] != '\0'; ++i) {
        print_char(str[i], false, color);
    }
}

void print_vector(const vector<char>& v, int color) { // Default arg in header
    for (size_t i = 0; i < v.size(); ++i) {
        print_char(v[i], false, color);
    }
}

void print_uint_base(unsigned long long n, int base, int color, bool print_prefix) {
    char buffer[65];
    int i = 0;
    const char* digits_map = "0123456789abcdef"; // Renamed 'digits' to avoid conflict if there's a global var

    if (base < 2 || base > 16) {
        print_string("[Invalid Base]", VGA_COLOR_LIGHT_RED);
        return;
    }

    if (n == 0) {
        buffer[i++] = '0';
    } else {
        while (n > 0) {
            buffer[i++] = digits_map[n % base];
            n /= base;
        }
    }

    if (print_prefix) {
        if (base == 16) { buffer[i++] = 'x'; buffer[i++] = '0'; }
        else if (base == 2) { buffer[i++] = 'b'; buffer[i++] = '0'; }
    }

    buffer[i] = '\0';

    int start = 0, end = i - 1;
    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
    print_string(buffer, color);
}

void print_int(long long n, int color) { // Default arg in header
    if (n == 0) {
        print_char('0', false, color);
        return;
    }
    if (n < 0) {
        print_char('-', false, color);
        // Special case for LLONG_MIN
        if (n == (-9223372036854775807LL - 1LL)) {
            print_string("9223372036854775808", color); // Print as string
            return;
        }
        n = -n; // Make n positive
    }
    print_uint_base((unsigned long long)n, 10, color, false);
}

void print_hex(uint64_t n, int color) { // Default arg in header
    print_uint_base(n, 16, color, true);
}

void print_hex32(uint32_t n, int color) { // Default arg in header
    print_uint_base(n, 16, color, true);
}

// --- Keyboard Input Function Definitions ---

char scancode_to_ascii(uint8_t scancode, bool shift_pressed) {
    if (scancode >= KEY_LIMIT) {
        return 0;
    }
    // Assumes scancode_ascii_normal and scancode_ascii_shifted are accessible
    // (e.g., via #include "consts.h" where they are extern char[])
    return shift_pressed ? scancode_ascii_shifted[scancode] : scancode_ascii_normal[scancode];
}

uint8_t scankey() {
    uint8_t scancode; // No need to initialize to 0, will be overwritten
    while (true) {
        if (inb(0x64) & 0x1) { // Check status port 0x64, bit 0 (output buffer full)
            scancode = inb(0x60); // Read data from port 0x60
            // PS/2 typically sends scancodes > 0.
            // A 0 might indicate an error or be part of a multi-byte sequence (not handled here).
            // For simplicity, we assume valid single-byte scancodes are non-zero.
            if (scancode != 0) { // Basic check, could be more robust
                return scancode;
            }
        }
        // Consider a short delay or 'pause' instruction if this loop consumes too much CPU
        // asm volatile("pause"); // Especially if interrupts are disabled for long periods.
    }
}

void input(vector<char>& v, int color) { // Default arg in header
    bool left_shift_pressed = false;
    bool right_shift_pressed = false;

    v.clear(); // Ensure vector is empty at start

    // Store initial cursor y, in case input spans multiple lines (not fully handled here but good for backspace)
    uint16_t initial_cursor_y = cursor_y; 
    uint16_t initial_cursor_x = cursor_x;


    while (true) {
        uint16_t temp_cursor_x = cursor_x; // Save current cursor for drawing temp '_'
        uint16_t temp_cursor_y = cursor_y;
        
        // Draw temporary cursor if current position is valid
        if (temp_cursor_y < VGA_HEIGHT && temp_cursor_x < VGA_WIDTH) {
            print_char('_', true, VGA_COLOR_DARK_GREY);
        }

        uint8_t scancode = scankey();

        // Erase temporary cursor (unless backspace, which handles its own screen update)
        if (scancode != BACKSPACE) {
            if (temp_cursor_y < VGA_HEIGHT && temp_cursor_x < VGA_WIDTH) {
                 uint16_t pos = temp_cursor_y * VGA_WIDTH + temp_cursor_x;
                 vga_buffer[pos] = (uint16_t)((VGA_DEFAULT_COLOR << 8) | ' '); // Erase with default color space
            }
        }

        switch (scancode) {
            case ENTER:
                return; // Input finished

            case SHIFT_PRESSED_LEFT:  left_shift_pressed = true; break;
            case SHIFT_RELEASED_LEFT: left_shift_pressed = false; break;
            case SHIFT_PRESSED_RIGHT: right_shift_pressed = true; break;
            case SHIFT_RELEASED_RIGHT:right_shift_pressed = false; break;

            case BACKSPACE:
                if (!v.empty()) {
                    v.pop_back();

                    // Move screen cursor back
                    if (cursor_x > 0) {
                        cursor_x--;
                    } else if (cursor_y > initial_cursor_y) { // Can go to previous line if input wrapped
                        cursor_y--;
                        cursor_x = VGA_WIDTH - 1;
                    } else if (cursor_y == initial_cursor_y && cursor_x > initial_cursor_x) {
                        // On the initial line, don't go before start of input
                        cursor_x--;
                    } else {
                        // At the very start of the input line, do nothing with cursor
                        // but ensure the temp '_' is erased if it was there
                        if (temp_cursor_y < VGA_HEIGHT && temp_cursor_x < VGA_WIDTH) {
                             uint16_t pos = temp_cursor_y * VGA_WIDTH + temp_cursor_x;
                             vga_buffer[pos] = (uint16_t)((VGA_DEFAULT_COLOR << 8) | ' ');
                        }
                        break; 
                    }
                    // Erase char on screen by printing a space at the new cursor position
                    print_char(' ', true, VGA_DEFAULT_COLOR); // Use default color for erasing
                } else {
                    // Input buffer is empty, just erase the temp cursor if it was drawn
                     if (temp_cursor_y < VGA_HEIGHT && temp_cursor_x < VGA_WIDTH) {
                        uint16_t pos = temp_cursor_y * VGA_WIDTH + temp_cursor_x;
                        vga_buffer[pos] = (uint16_t)((VGA_DEFAULT_COLOR << 8) | ' ');
                    }
                }
                break;

            default:
                // Process only key presses (make codes), ignore key releases (break codes, usually scancode | 0x80)
                if (!(scancode & 0x80)) {
                    bool shift_pressed = left_shift_pressed || right_shift_pressed;
                    char c = scancode_to_ascii(scancode, shift_pressed);
                    if (c != 0) { // If it's a printable character (not null)
                        // Optional: Add a MAX_INPUT_LENGTH check here
                        // if (v.size() < MAX_INPUT_LENGTH) {
                        v.push_back(c);
                        print_char(c, false, color); // Print char and advance cursor
                        // }
                    }
                }
                break;
        }
    }
}