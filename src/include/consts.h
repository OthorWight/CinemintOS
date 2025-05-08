#ifndef CONSTS_H
#define CONSTS_H

#include <stdint.h> // For uint16_t, uint8_t

// --- VGA Colors Enum ---
// This is a type definition, so it's fine in a header (with include guards).
enum vga_color
{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,       // Renamed from LIGHT_BROWN, as YELLOW is more common & needed
    VGA_COLOR_WHITE = 15,
};

// --- Keyboard Scancode Defines ---
// These are preprocessor macros, so they are fine in a header.
#define SHIFT_PRESSED_LEFT  0x2A
#define SHIFT_RELEASED_LEFT 0xAA
#define SHIFT_PRESSED_RIGHT 0x36
#define SHIFT_RELEASED_RIGHT 0xB6
#define BACKSPACE           0x0E
#define ENTER               0x1C // Common scancode for Enter key (Set 1)
#define KEY_LIMIT           59   // Ensure this matches the size of your scancode arrays

// --- VGA Text Mode Constants (Declarations) ---
// Use 'extern' to declare that these variables are defined elsewhere.
extern const uint16_t VGA_WIDTH;
extern const uint16_t VGA_HEIGHT;
extern const uint16_t VGA_DEFAULT_COLOR; // Renamed to avoid conflict if VGA_COLOR is used as a variable name
extern int vga_mode;                     // If this is truly global state, extern is correct.

// --- Scancode to ASCII Mappings (Declarations) ---
// Use 'extern' for these arrays as well.
extern const char scancode_ascii_normal[KEY_LIMIT];
extern const char scancode_ascii_shifted[KEY_LIMIT];

#endif // CONSTS_H