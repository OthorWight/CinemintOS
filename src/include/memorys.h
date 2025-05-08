#ifndef MEMORYS_H
#define MEMORYS_H

#include <stddef.h> // For size_t
#include <stdint.h> // For uint32_t, uint64_t, etc.

// --- Custom Memory Allocator Declarations ---
// Declare the memory pool and index as extern.
// The size of the pool can be a const extern if needed elsewhere,
// or just managed within memorys.cpp.
extern char memory_pool[]; // Actual size defined in .cpp
extern size_t pool_index;

// Declare operator new[] and delete[]
// The 'noexcept' specifier is important for delete operators and
// for new operators that are guaranteed not to throw (like this simple one).
// The warning you saw "operator new must not return NULL unless it is declared ‘throw()’"
// can be addressed by marking it noexcept if it never throws, or if using C++11 or later,
// you could use `throw(std::bad_alloc)` or `throw()` for older C++ if it could throw.
// Since this one returns nullptr on failure, `noexcept` is appropriate.
void* operator new[](size_t size) noexcept;
void operator delete[](void* ptr) noexcept;

// Consider adding a non-array version if you'll use `new MyType;`
// void* operator new(size_t size) noexcept;
// void operator delete(void* ptr) noexcept;


// --- Multiboot Structures ---
// These are type definitions, so they are fine in a header (with include guards).
struct multiboot_info
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4]; // Note: This field is complex if used. Often ignored.
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;      // Available if bit 12 of flags is set
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;      // 0 for indexed, 1 for direct RGB, 2 for text
    union {
        struct {
            uint32_t framebuffer_palette_addr;
            uint16_t framebuffer_palette_num_colors;
        } indexed;
        struct {
            uint8_t framebuffer_red_field_position;
            uint8_t framebuffer_red_mask_size;
            uint8_t framebuffer_green_field_position;
            uint8_t framebuffer_green_mask_size;
            uint8_t framebuffer_blue_field_position;
            uint8_t framebuffer_blue_mask_size;
        } rgb;
    } framebuffer_color_info; // Replaces your uint8_t color_info[6]; for clarity
};

// Memory map entry structure
struct mmap_entry
{
    uint32_t size;     // Size of this structure itself (doesn't include the +4 skip)
    uint64_t addr;     // Starting address of the memory region
    uint64_t len;      // Length of the memory region in bytes
    uint32_t type;     // Type of memory region (1 = available, other values = reserved/ACPI/etc.)
} __attribute__((packed)); // Ensure no padding

// --- External Variable from Bootloader ---
// This tells the C++ code that a symbol named mboot_info_ptr is defined elsewhere (likely boot.asm).
extern "C" uint32_t mboot_info_ptr;

// --- Function Declarations ---
// Declare the function to get total RAM.
uint32_t get_total_ram_mb(multiboot_info* mbi);

#endif // MEMORYS_H