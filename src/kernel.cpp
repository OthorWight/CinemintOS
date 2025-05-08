#include <stdint.h> // Standard for fixed-width integers

// OS-specific includes - These bring in DECLARATIONS
#include "include/consts.h"    // For VGA Colors, Keyboard scancode defines, KEY_LIMIT etc.
#include "include/vectors.h"   // For vector<char>
#include "include/memorys.h"   // For multiboot_info and memory functions/allocators
#include "include/screens.h"   // For cls() and screen-related externs (vga_buffer, cursor_x/y)
#include "include/io.h"        // For print_*, input(), inb, outw, etc.
#include "include/acpi.h"      // For acpi_init(), acpi_power_off(), and FADT extern

// --- Helper functions for string/vector operations ---
static bool streq_vec(const vector<char>& vec, const char* c_str) {
    size_t c_str_len = 0;
    while (c_str[c_str_len] != '\0') {
        c_str_len++;
    }
    if (vec.size() != c_str_len) {
        return false;
    }
    for (size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] != c_str[i]) {
            return false;
        }
    }
    return true;
}

static bool strstarts_vec(const vector<char>& vec, const char* prefix) {
    size_t prefix_len = 0;
    while (prefix[prefix_len] != '\0') {
        prefix_len++;
    }
    if (vec.size() < prefix_len) {
        return false;
    }
    for (size_t i = 0; i < prefix_len; ++i) {
        if (vec[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

static void print_vector_char_range(const vector<char>& vec, size_t start_index, uint8_t color) {
    if (start_index >= vec.size()) {
        return;
    }
    const int MAX_ARG_LEN = 256;
    char buffer[MAX_ARG_LEN];
    size_t len_to_print = vec.size() - start_index;

    if (len_to_print >= MAX_ARG_LEN) {
        len_to_print = MAX_ARG_LEN - 1;
    }
    for (size_t i = 0; i < len_to_print; ++i) {
        buffer[i] = vec[start_index + i];
    }
    buffer[len_to_print] = '\0';
    print_string(buffer, color);
}

// Basic string to integer conversion.
static long long simple_str_to_long(const vector<char>& vec, size_t& index) {
    long long res = 0;
    bool is_negative = false;
    bool found_digit_or_sign = false;

    // Skip leading spaces before the number
    while (index < vec.size() && vec[index] == ' ') {
        index++;
    }

    // Check for sign (simple version: only at the beginning of the number part)
    if (index < vec.size() && vec[index] == '-') {
        is_negative = true;
        index++;
        found_digit_or_sign = true;
    } else if (index < vec.size() && vec[index] == '+') {
        index++;
        found_digit_or_sign = true;
    }
    
    bool found_digit = false;
    size_t start_digit_index = index;
    while (index < vec.size() && vec[index] >= '0' && vec[index] <= '9') {
        res = res * 10 + (vec[index] - '0');
        index++;
        found_digit = true;
    }

    if (!found_digit && found_digit_or_sign && !is_negative) { // e.g. "calc +"
         // This is tricky, maybe means just a sign was found without digits.
         // For simplicity, treat as error or 0. Let's assume it means the number parsing ended.
    } else if (!found_digit && !found_digit_or_sign) { // No digits or sign found at all
        // No number found where one was expected
        return 0; // Or some error indicator
    }


    return is_negative ? -res : res;
}


// --- Kernel Entry Point ---
extern "C" void kernel_main(multiboot_info* mbi) {
    cls();

    print_string("Howdy! Welcome to Cinemint OS!\n", VGA_COLOR_LIGHT_CYAN);
    print_string("----------------------------------\n", VGA_COLOR_LIGHT_CYAN);

    print_string("Initializing ACPI...\n", VGA_COLOR_WHITE);
    acpi_init();
    print_string("----------------------------------\n", VGA_COLOR_LIGHT_CYAN);

    if (mbi) {
        if (mbi->flags & (1 << 0)) {
            print_string("Free Memory (Lower KB): ", VGA_COLOR_WHITE); print_int(mbi->mem_lower); print_string(" KB\n", VGA_COLOR_WHITE);
            print_string("Free Memory (Upper KB): ", VGA_COLOR_WHITE); print_int(mbi->mem_upper); print_string(" KB\n", VGA_COLOR_WHITE);
            print_string("Total Free Memory (Basic): ", VGA_COLOR_WHITE); print_int(mbi->mem_lower + mbi->mem_upper); print_string(" KB\n", VGA_COLOR_WHITE);
        }
        uint32_t ram_from_mmap = get_total_ram_mb(mbi);
        if (ram_from_mmap > 0) {
             print_string("Total RAM from MMAP: ", VGA_COLOR_WHITE); print_int(ram_from_mmap); print_string(" MB\n", VGA_COLOR_WHITE);
        } else if (!(mbi->flags & (1 << 6))) { // Only print this if mmap flag wasn't set
            print_string("MMAP info not explicitly available via flags for detailed RAM count.\n", VGA_COLOR_YELLOW);
        }
        print_char('\n');
    } else {
        print_string("Multiboot info not available (initial print).\n", VGA_COLOR_LIGHT_RED);
    }

    print_string("Type 'help' for available commands.\n\n", VGA_COLOR_WHITE);

    vector<char> input_buffer;
    const char* prompt = "Cinemint> ";

    while (true) {
        print_string(prompt, VGA_COLOR_GREEN);
        input(input_buffer);
        print_char('\n');

        if (input_buffer.empty()) {
            continue;
        }

        if (streq_vec(input_buffer, "help")) {
            print_string("Available commands:\n", VGA_COLOR_WHITE);
            print_string("  help             - Show this help message\n", VGA_COLOR_WHITE);
            print_string("  cls              - Clear the screen\n", VGA_COLOR_WHITE);
            print_string("  echo [text]      - Print [text] to the screen\n", VGA_COLOR_WHITE);
            print_string("  calc <n1> <op> <n2> - Basic calculator (+, -, *, /)\n", VGA_COLOR_WHITE); // Added calc
            print_string("  shutdown         - Power off the system via ACPI S5\n", VGA_COLOR_WHITE);
        } else if (streq_vec(input_buffer, "cls")) {
            cls();
        } else if (strstarts_vec(input_buffer, "echo ")) {
            if (input_buffer.size() > 5) {
                print_vector_char_range(input_buffer, 5, VGA_COLOR_WHITE);
                print_char('\n');
            } else {
                print_char('\n');
            }
        } else if (strstarts_vec(input_buffer, "calc ")) {
            size_t index = 5; // Start parsing after "calc "
            long long num1 = 0;
            long long num2 = 0;
            char op = 0;
            long long result = 0;
            bool error = false;

            // Parse num1
            num1 = simple_str_to_long(input_buffer, index);

            // Skip spaces to find operator
            while (index < input_buffer.size() && input_buffer[index] == ' ') {
                index++;
            }

            // Parse operator
            if (index < input_buffer.size()) {
                op = input_buffer[index];
                index++;
            } else {
                error = true;
            }

            // Skip spaces to find num2
            while (index < input_buffer.size() && input_buffer[index] == ' ') {
                index++;
            }
            
            // Parse num2
            if (!error && index < input_buffer.size()) {
                 // Check if the rest of the string after operator and spaces is a valid start for a number
                if (input_buffer[index] >= '0' && input_buffer[index] <= '9' || input_buffer[index] == '-' || input_buffer[index] == '+') {
                    num2 = simple_str_to_long(input_buffer, index);
                } else {
                    error = true; // Invalid character where num2 should start
                }
            } else if (!error) { // Reached end of input before num2
                 error = true;
            }


            // Perform calculation
            if (!error) {
                switch (op) {
                    case '+': result = num1 + num2; break;
                    case '-': result = num1 - num2; break;
                    case '*': result = num1 * num2; break;
                    case '/':
                        if (num2 == 0) {
                            print_string("Error: Division by zero.\n", VGA_COLOR_LIGHT_RED);
                            error = true;
                        } else {
                            result = num1 / num2;
                        }
                        break;
                    default:
                        print_string("Error: Invalid operator '", VGA_COLOR_LIGHT_RED);
                        print_char(op, false, VGA_COLOR_LIGHT_RED);
                        print_string("'. Use +, -, *, /.\n", VGA_COLOR_LIGHT_RED);
                        error = true;
                        break;
                }
            }

            if (!error) {
                print_int(result);
                print_char('\n');
            } else {
                if (op == 0 || (index <= 5 && num1 == 0 && op ==0) ) { // Very basic check for insufficient args
                     print_string("Usage: calc <num1> <op> <num2>\n", VGA_COLOR_YELLOW);
                }
                // Specific error messages for operator/division by zero are printed above.
            }

        } else if (streq_vec(input_buffer, "shutdown")) {
            acpi_power_off();
            print_string("ACPI shutdown sequence problem. System did not power off.\n", VGA_COLOR_LIGHT_RED);
        } else {
            print_string("Unknown command: ", VGA_COLOR_LIGHT_RED);
            print_vector_char_range(input_buffer, 0, VGA_COLOR_LIGHT_RED);
            print_char('\n');
        }
    }
}