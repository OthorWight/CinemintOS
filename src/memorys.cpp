#include "include/memorys.h"
// Potentially include "io.h" if you need print_string for debugging in here.
// #include "include/io.h"

// --- Custom Memory Allocator Definitions ---
// Define the actual memory pool and its size.
// The 'alignas(16)' can be useful if you allocate types that need specific alignment.
alignas(16) char memory_pool[1024 * 1024]; // 1MB pool
size_t pool_index = 0; // Initialized to 0

// Max size for the pool, can be used for boundary check
const size_t MEMORY_POOL_SIZE = sizeof(memory_pool);

void* operator new[](size_t size) noexcept {
    // Basic alignment: ensure pool_index is aligned to a multiple of a common alignment (e.g., 16 or sizeof(void*))
    // This is a very simple alignment strategy.
    size_t alignment = 16; // Or sizeof(void*)
    size_t remainder = pool_index % alignment;
    if (remainder != 0) {
        pool_index += (alignment - remainder);
    }

    if (pool_index + size > MEMORY_POOL_SIZE) {
        // Optional: print an out-of-memory message if print functions are available
        // print_string("Kernel heap out of memory (new[])!\n", VGA_COLOR_LIGHT_RED);
        return nullptr; // Out of memory
    }
    void* ptr = &memory_pool[pool_index];
    pool_index += size;
    return ptr;
}

void operator delete[](void* ptr) noexcept {
    // For a simple bump allocator, delete is usually a no-op.
    // More advanced allocators would free the memory here.
    (void)ptr; // Suppress unused parameter warning
}

// If you added declarations for non-array new/delete in the header:
/*
void* operator new(size_t size) noexcept {
    size_t alignment = 16;
    size_t remainder = pool_index % alignment;
    if (remainder != 0) {
        pool_index += (alignment - remainder);
    }

    if (pool_index + size > MEMORY_POOL_SIZE) {
        return nullptr;
    }
    void* ptr = &memory_pool[pool_index];
    pool_index += size;
    return ptr;
}

void operator delete(void* ptr) noexcept {
    (void)ptr;
}
*/


// --- Function Definitions ---
uint32_t get_total_ram_mb(multiboot_info* mbi) {
    if (!mbi) return 0; // Safety check

    // Check if basic memory info is available (bit 0 of flags)
    if (!(mbi->flags & (1 << 0))) { // Bit 0 for mem_lower/upper
        // Optional: print a warning if print functions are available
        // print_string("Warning: Basic memory info (mem_lower/upper) not available from Multiboot.\n");
        return 0; // Memory info not available
    }

    // If memory map is available (bit 6 of flags), use it for more accurate info
    if (mbi->flags & (1 << 6)) { // Bit 6 for mmap
        uint64_t total_ram_bytes = 0;
        // Ensure mmap_addr is valid before dereferencing
        if (mbi->mmap_addr == 0 || mbi->mmap_length == 0) {
             // print_string("Warning: Memory map address or length is zero.\n");
             // Fallback to basic mem_lower/upper if mmap is invalid
             return (mbi->mem_lower + mbi->mem_upper) / 1024;
        }

        mmap_entry* entry = (mmap_entry*)((uintptr_t)mbi->mmap_addr);
        // The end condition for the loop should be based on iterating through mmap_length bytes
        uint32_t current_offset = 0;
        while (current_offset < mbi->mmap_length) {
            // Type 1 indicates available RAM
            if (entry->type == 1) {
                total_ram_bytes += entry->len;
            }
            // Go to next entry: the size of the mmap_entry structure itself + 'size' field
            // The 'size' field indicates how large this mmap_entry is, so the next one starts after it.
            // The Multiboot spec says: "The `size` field contains the size of the associated structure in bytes,
            // which can be greater than the minimum of 20 bytes. ... a boot loader should skip `size` bytes to get the next structure."
            // However, it's common to see `entry->size` being the size of the fixed part (20 or 24 bytes), and the iteration
            // actually being `(mmap_entry*) ((uintptr_t)entry + entry->size + sizeof(uint32_t))` if size only covers up to `type`.
            // Let's assume `entry->size` is the size of the *current* entry structure.
            // A safer way:
            if (entry->size == 0) { // Avoid infinite loop if size is 0
                // print_string("Warning: mmap entry size is 0. Aborting mmap scan.\n");
                break;
            }
            current_offset += entry->size + 4; // The '+4' is because the 'size' field itself is part of the entry,
                                               // but older interpretations might see 'size' not including itself.
                                               // The most common interpretation is that 'size' is the size of the
                                               // entry, and you advance by 'size' to get to the next entry.
                                               // But many bootloaders (like GRUB) make `entry->size` not include itself for iteration,
                                               // requiring `entry = (mmap_entry*)((uintptr_t)entry + entry->size + 4);`

            // The most robust GRUB-like iteration:
            entry = (mmap_entry*) ((uintptr_t)entry + entry->size + sizeof(uint32_t)); // entry->size + itself (4 bytes)

        }
        return total_ram_bytes / (1024 * 1024); // Convert bytes to MB
    } else {
        // Fallback to basic mem_lower/upper if mmap is not available
        // mem_lower is memory below 1MB, mem_upper is memory above 1MB (up to 4GB). Both in KB.
        return (mbi->mem_lower + mbi->mem_upper) / 1024;
    }
}