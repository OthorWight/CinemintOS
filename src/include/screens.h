#ifndef SCREENS_H
#define SCREENS_H

#include <stdint.h> // For uint16_t

// --- Screen Global Variable Declarations ---
// Declare these as 'extern'. Their definitions will be in screen.cpp.
// 'volatile' is important for vga_buffer as it's memory-mapped I/O.
extern volatile uint16_t* vga_buffer;
extern uint16_t cursor_x;
extern uint16_t cursor_y;

// --- Screen Function Declarations ---
// Only declare the functions here. Definitions go in screen.cpp.

/**
 * @brief Clears the entire VGA text screen and resets the cursor.
 */
void cls();

/**
 * @brief Scrolls the screen content up by one line.
 *        The bottom-most line is cleared.
 * @param buffer_to_scroll Pointer to the VGA buffer to operate on.
 *                         Typically, this will be the global vga_buffer.
 */
void scroll_screen(volatile uint16_t* buffer_to_scroll);
// Alternatively, if scroll_screen always operates on the global vga_buffer:
// void scroll_screen(); // And the implementation in screen.cpp would use the global.

#endif // SCREENS_H