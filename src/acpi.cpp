#include "include/acpi.h"
#include "include/io.h"     // For print_string, print_hex32, print_int, print_char, outb, inw, etc.
#include "include/consts.h" // For VGA_COLOR_*, size_t from vectors.h via io.h might be used if not careful
#include "include/vectors.h" // For size_t (if your size_t is defined here and not pulled in via other headers)

// --- Global Variable Definition ---
FADT* g_fadt = nullptr;

// --- Helper: Custom memcmp ---
// This is static, so its scope is limited to this file (acpi.cpp).
static int memcmp_custom(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    for (size_t i = 0; i < num; ++i) {
        if (p1[i] < p2[i]) return -1;
        if (p1[i] > p2[i]) return 1;
    }
    return 0;
}


// --- ACPI Function Definitions ---

bool validate_acpi_sdt_checksum(ACPISDTHeader* tableHeader) {
    if (!tableHeader) return false; // Basic null check
    unsigned char sum = 0;
    for (uint32_t i = 0; i < tableHeader->Length; i++) {
        sum += ((char*)tableHeader)[i];
    }
    return sum == 0;
}

void* find_rsdp() {
    // Search in EBDA (Extended BIOS Data Area)
    // The EBDA pointer is at 0x40E in BDA (BIOS Data Area)
    uint16_t ebda_segment = *(uint16_t*)0x40E; // Direct memory access
    // Check if EBDA segment is valid (e.g., not 0)
    if (ebda_segment != 0) {
        unsigned char* ebda_ptr = (unsigned char*)((uintptr_t)ebda_segment << 4);
        for (unsigned int i = 0; i < 1024; i += 16) { // Search first 1KB of EBDA
            if (memcmp_custom(ebda_ptr + i, "RSD PTR ", 8) == 0) {
                RSDPDescriptor* rsdp = (RSDPDescriptor*)(ebda_ptr + i);
                unsigned char sum = 0;
                for (size_t j = 0; j < sizeof(RSDPDescriptor); ++j) { // ACPI 1.0 size (20 bytes)
                    sum += ((unsigned char*)rsdp)[j];
                }
                if (sum == 0) { // Valid checksum for ACPI 1.0 part
                    if (rsdp->Revision >= 2 && rsdp->Revision != 0) { // ACPI 2.0+ (Revision can be 0 for 1.0, or >=2 for 2.0+)
                        RSDPDescriptor20* rsdp20 = (RSDPDescriptor20*)(ebda_ptr + i);
                        // Check if length is reasonable for RSDP 2.0 struct
                        if (rsdp20->Length >= sizeof(RSDPDescriptor20)) {
                            sum = 0; // Recalculate for the full 2.0 structure
                            for (size_t k = 0; k < rsdp20->Length; ++k) {
                                sum += ((unsigned char*)rsdp20)[k];
                            }
                            if (sum == 0) {
                                print_string("RSDP 2.0+ found in EBDA.\n", VGA_COLOR_WHITE);
                                return rsdp20;
                            }
                        }
                    } else { // ACPI 1.0 (Revision 0)
                        print_string("RSDP 1.0 found in EBDA.\n", VGA_COLOR_WHITE);
                        return rsdp;
                    }
                }
            }
        }
    }


    // Search in main BIOS area (0xE0000 to 0xFFFFF)
    for (unsigned char* ptr = (unsigned char*)0xE0000; (uintptr_t)ptr < 0xFFFFF; ptr += 16) {
        if (memcmp_custom(ptr, "RSD PTR ", 8) == 0) {
            RSDPDescriptor* rsdp = (RSDPDescriptor*)ptr;
            unsigned char sum = 0;
            for (size_t j = 0; j < sizeof(RSDPDescriptor); ++j) { // ACPI 1.0 size
                sum += ((unsigned char*)rsdp)[j];
            }
            if (sum == 0) { // Valid checksum for ACPI 1.0 part
                 if (rsdp->Revision >= 2 && rsdp->Revision != 0) { // ACPI 2.0+
                    RSDPDescriptor20* rsdp20 = (RSDPDescriptor20*)ptr;
                    if (rsdp20->Length >= sizeof(RSDPDescriptor20)) {
                        sum = 0; // Recalculate for the full 2.0 structure
                        for (size_t k = 0; k < rsdp20->Length; ++k) {
                            sum += ((unsigned char*)rsdp20)[k];
                        }
                        if (sum == 0) {
                            print_string("RSDP 2.0+ found in BIOS area.\n", VGA_COLOR_WHITE);
                            return rsdp20;
                        }
                    }
                } else { // ACPI 1.0
                    print_string("RSDP 1.0 found in BIOS area.\n", VGA_COLOR_WHITE);
                    return rsdp;
                }
            }
        }
    }

    print_string("RSDP not found.\n", VGA_COLOR_LIGHT_RED);
    return nullptr;
}

ACPISDTHeader* find_sdt_from_rsdp(void* rsdp_ptr, const char* signature) {
    if (!rsdp_ptr || !signature) { // Check signature for null as well
        return nullptr;
    }

    RSDPDescriptor* rsdp_v1 = (RSDPDescriptor*)rsdp_ptr;
    ACPISDTHeader* sdt_root_table = nullptr;
    int entries = 0;
    bool use_xsdt = (rsdp_v1->Revision >= 2 && rsdp_v1->Revision != 0); // Revision 0 is 1.0, >=2 is 2.0+

    if (use_xsdt) {
        RSDPDescriptor20* rsdp_v2 = (RSDPDescriptor20*)rsdp_ptr;
        if (rsdp_v2->XsdtAddress == 0) { // Check for null XSDT address
            print_string("XSDT address is NULL in RSDP v2.0+, falling back to RSDT if available.\n", VGA_COLOR_YELLOW);
            use_xsdt = false; // Fallback to RSDT
        } else {
            sdt_root_table = (ACPISDTHeader*)((uintptr_t)rsdp_v2->XsdtAddress);
            if (!sdt_root_table || !validate_acpi_sdt_checksum(sdt_root_table)) {
                print_string("XSDT invalid or checksum failed.\n", VGA_COLOR_LIGHT_RED);
                return nullptr;
            }
            // Length of XSDT - size of header = size of pointer array
            // Each entry in XSDT is 8 bytes (uint64_t)
            entries = (sdt_root_table->Length - sizeof(ACPISDTHeader)) / 8;
            print_string("Using XSDT. Entries: ", VGA_COLOR_WHITE); print_int(entries); print_char('\n');
        }
    }
    
    // If not using XSDT (either ACPI 1.0 or XSDT fallback)
    if (!use_xsdt) { 
        if (rsdp_v1->RsdtAddress == 0) { // Check for null RSDT address
             print_string("RSDT address is NULL.\n", VGA_COLOR_LIGHT_RED);
             return nullptr;
        }
        sdt_root_table = (ACPISDTHeader*)((uintptr_t)rsdp_v1->RsdtAddress);
        if (!sdt_root_table || !validate_acpi_sdt_checksum(sdt_root_table)) {
             print_string("RSDT invalid or checksum failed.\n", VGA_COLOR_LIGHT_RED);
             return nullptr;
        }
        // Length of RSDT - size of header = size of pointer array
        // Each entry in RSDT is 4 bytes (uint32_t)
        entries = (sdt_root_table->Length - sizeof(ACPISDTHeader)) / 4;
        print_string("Using RSDT. Entries: ", VGA_COLOR_WHITE); print_int(entries); print_char('\n');
    }


    for (int i = 0; i < entries; i++) {
        ACPISDTHeader* h;
        if (use_xsdt) {
            uint64_t* ptr_array = (uint64_t*)((char*)sdt_root_table + sizeof(ACPISDTHeader));
            h = (ACPISDTHeader*)((uintptr_t)ptr_array[i]);
        } else {
            uint32_t* ptr_array = (uint32_t*)((char*)sdt_root_table + sizeof(ACPISDTHeader));
            h = (ACPISDTHeader*)((uintptr_t)ptr_array[i]);
        }

        if (!h) { // Check for null pointer in the table entries
            print_string("Null SDT pointer encountered in (X)RSDT.\n", VGA_COLOR_YELLOW);
            continue; 
        }

        // Use the 'signature' parameter here
        if (memcmp_custom(h->Signature, signature, 4) == 0) {
            if (validate_acpi_sdt_checksum(h)) {
                print_string("Found table: ", VGA_COLOR_GREEN);
                // Create a null-terminated buffer for the signature for printing
                char sig_buf[5];
                sig_buf[0] = signature[0]; sig_buf[1] = signature[1];
                sig_buf[2] = signature[2]; sig_buf[3] = signature[3];
                sig_buf[4] = '\0';
                print_string(sig_buf, VGA_COLOR_GREEN); print_char('\n');
                return h;
            } else {
                print_string("Found table '", VGA_COLOR_LIGHT_RED);
                char sig_buf[5];
                sig_buf[0] = signature[0]; sig_buf[1] = signature[1];
                sig_buf[2] = signature[2]; sig_buf[3] = signature[3];
                sig_buf[4] = '\0';
                print_string(sig_buf, VGA_COLOR_LIGHT_RED);
                print_string("' but its checksum failed.\n", VGA_COLOR_LIGHT_RED);
            }
        }
    }

    print_string("Table '", VGA_COLOR_LIGHT_RED);
    char sig_buf[5];
    sig_buf[0] = signature[0]; sig_buf[1] = signature[1];
    sig_buf[2] = signature[2]; sig_buf[3] = signature[3];
    sig_buf[4] = '\0';
    print_string(sig_buf, VGA_COLOR_LIGHT_RED);
    print_string("' not found in (X)RSDT.\n", VGA_COLOR_LIGHT_RED);
    return nullptr;
}

void acpi_init() {
    void* rsdp = find_rsdp(); // find_rsdp() prints messages
    if (!rsdp) {
        // find_rsdp already printed "RSDP not found."
        print_string("ACPI initialization failed: RSDP not found.\n", VGA_COLOR_LIGHT_RED);
        return;
    }

    g_fadt = (FADT*)find_sdt_from_rsdp(rsdp, "FACP"); // "FACP" is the signature for FADT
    if (g_fadt) {
        print_string("FADT found. SCI_Interrupt: ", VGA_COLOR_GREEN);
        print_int(g_fadt->SCI_Interrupt, VGA_COLOR_GREEN);
        print_string(", PM1aCtrlBlk: 0x", VGA_COLOR_GREEN);
        if (g_fadt->PM1aControlBlock) {
            print_hex32(g_fadt->PM1aControlBlock, VGA_COLOR_GREEN);
        } else {
            print_string("N/A", VGA_COLOR_YELLOW);
        }
        print_char('\n');

        // Enable ACPI mode if SMI_CommandPort, AcpiEnable, and PM1aEventBlock are valid
        if (g_fadt->SMI_CommandPort && g_fadt->AcpiEnable && g_fadt->PM1aEventBlock) {
            print_string("Attempting to enable ACPI mode (writing to SMI_CMD 0x", VGA_COLOR_WHITE);
            print_hex32(g_fadt->SMI_CommandPort, VGA_COLOR_WHITE);
            print_string(" value 0x", VGA_COLOR_WHITE);
            print_hex32(g_fadt->AcpiEnable, VGA_COLOR_WHITE);
            print_string(")\n", VGA_COLOR_WHITE);

            outb(g_fadt->SMI_CommandPort, g_fadt->AcpiEnable);

            int timeout_counter = 0;
            const int MAX_TIMEOUT_ITERATIONS = 500000; // Adjust as needed
            bool sci_enabled = false;
            while (timeout_counter < MAX_TIMEOUT_ITERATIONS) {
                if ((inw(g_fadt->PM1aEventBlock) & 1)) { // SCI_EN is bit 0 of PM1 Status Register
                    sci_enabled = true;
                    break;
                }
                // Basic delay - in a real kernel, this would be a proper timer delay or scheduler yield
                for(volatile int d = 0; d < 100; ++d); 
                timeout_counter++;
            }

            if (sci_enabled) {
                 print_string("ACPI mode enabled (SCI_EN bit is set in PM1aEventBlock).\n", VGA_COLOR_GREEN);
            } else {
                print_string("Timeout or failure enabling ACPI mode (SCI_EN not set).\n", VGA_COLOR_LIGHT_RED);
                print_string("PM1aEventBlock (0x", VGA_COLOR_YELLOW);
                print_hex32(g_fadt->PM1aEventBlock, VGA_COLOR_YELLOW);
                print_string(") current value: 0x", VGA_COLOR_YELLOW);
                print_hex32(inw(g_fadt->PM1aEventBlock), VGA_COLOR_YELLOW); // Print current value of PM1a status
                print_char('\n');
            }
        } else {
            print_string("Cannot attempt to enable ACPI mode: SMI_CommandPort, AcpiEnable, or PM1aEventBlock is zero in FADT.\n", VGA_COLOR_YELLOW);
        }
    } else {
        // find_sdt_from_rsdp already printed "Table 'FACP' not found."
        print_string("ACPI initialization failed: FADT not found.\n", VGA_COLOR_LIGHT_RED);
    }
}

void acpi_power_off() {
    if (!g_fadt) {
        print_string("ACPI Shutdown: FADT not found.\n", VGA_COLOR_LIGHT_RED);
        return;
    }

    if (g_fadt->PM1aControlBlock == 0) { // Check if PM1aControlBlock port is valid
        print_string("ACPI Shutdown: PM1aControlBlock is not defined (zero) in FADT.\n", VGA_COLOR_LIGHT_RED);
        return;
    }

    // Sanity check: Is ACPI mode actually enabled? (SCI_EN bit)
    // This might not be strictly necessary if acpi_init() was successful, but good for robustness.
    if (g_fadt->PM1aEventBlock && !(inw(g_fadt->PM1aEventBlock) & 1)) {
         print_string("Warning (ACPI Shutdown): SCI_EN bit not set. Shutdown might fail.\n", VGA_COLOR_YELLOW);
    }

    print_string("Attempting ACPI S5 soft-off...\n", VGA_COLOR_YELLOW);
    
    uint16_t pm1a_cnt_val = SLP_TYP_S5_PM1_CNT | SLP_EN_PM1_CNT;
    print_string("Writing 0x", VGA_COLOR_WHITE);
    print_hex32(pm1a_cnt_val, VGA_COLOR_WHITE); // print_hex32 for uint16_t is fine
    print_string(" to PM1a_CNT (port 0x", VGA_COLOR_WHITE);
    print_hex32(g_fadt->PM1aControlBlock, VGA_COLOR_WHITE);
    print_string(")\n", VGA_COLOR_WHITE);

    asm volatile("cli"); // Disable interrupts

    outw(g_fadt->PM1aControlBlock, pm1a_cnt_val);

    // If PM1b_CNT_BLK is valid and different from PM1a, write to it as well
    if (g_fadt->PM1bControlBlock != 0 && g_fadt->PM1bControlBlock != g_fadt->PM1aControlBlock) {
        print_string("Writing 0x", VGA_COLOR_WHITE);
        print_hex32(pm1a_cnt_val, VGA_COLOR_WHITE);
        print_string(" to PM1b_CNT (port 0x", VGA_COLOR_WHITE);
        print_hex32(g_fadt->PM1bControlBlock, VGA_COLOR_WHITE);
        print_string(")\n", VGA_COLOR_WHITE);
        outw(g_fadt->PM1bControlBlock, pm1a_cnt_val);
    }

    print_string("Shutdown command sent. System should power off.\n", VGA_COLOR_YELLOW);
    print_string("If not, this ACPI S5 method may be unsupported or require DSDT parsing for SLP_TYPa/b values.\n", VGA_COLOR_YELLOW);

    // Halt indefinitely
    while (true) {
        asm volatile("hlt");
    }
}