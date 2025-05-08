#include "include/consts.h" // Include the header to ensure consistency

// Definitions for variables declared extern in consts.h
const uint16_t VGA_WIDTH = 80;
const uint16_t VGA_HEIGHT = 25;
const uint16_t VGA_DEFAULT_COLOR = VGA_COLOR_LIGHT_GREY; // Use the enumeral
int vga_mode = 0;

// Scancode to ASCII mappings (Definitions)
// Ensure these arrays match the KEY_LIMIT in consts.h
const char scancode_ascii_normal[KEY_LIMIT] = {
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', BACKSPACE, '\t', // 0-15 (Tab, Backspace)
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, // 16-29 (Enter, LCtrl)
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', // 30-43 (LShift)
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0,  // 44-58 (RShift, Alt, Space, CapsLock)
    // Your previous arrays had some zeros that looked like keycodes, 
    // and others as placeholders. I've tried to align with common scancode set 1:
    // 0 = Null (No char)
    // Scancode 0x0E = BACKSPACE
    // Scancode 0x0F = TAB
    // Scancode 0x1C = ENTER
    // Scancode 0x29 = ` (grave accent)
    // Scancode 0x38 = Alt (usually 0 for char mapping)
    // Scancode 0x39 = Space
    // Scancode 0x3A = CapsLock (usually 0 for char mapping)
    // Please verify these against your intended scancode set 1 mappings.
    // The scancodes 0, 15, 29, 42, 54, 56, 57, 58+ are often non-printable or special.
};

const char scancode_ascii_shifted[KEY_LIMIT] = {
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', BACKSPACE, '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' ', 0,
};