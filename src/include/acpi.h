#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
// You might need to include your io.h here if functions like print_string are used
// in the ACPI implementation details, or ensure they are globally available.
// #include "io.h" // If ACPI implementation functions print status messages

// --- ACPI Structure Definitions ---
// Pack structures tightly to match memory layout
#pragma pack(push, 1)

// Root System Description Pointer (RSDP) Descriptor for ACPI 1.0
struct RSDPDescriptor {
    char Signature[8];      // "RSD PTR "
    uint8_t Checksum;       // To make sum of all bytes 0
    char OEMID[6];
    uint8_t Revision;       // 0 for ACPI 1.0, 2 for ACPI 2.0+
    uint32_t RsdtAddress;   // Physical address of RSDT
};

// RSDP Descriptor for ACPI 2.0 and later (extends RSDPDescriptor)
struct RSDPDescriptor20 {
    RSDPDescriptor firstPart; // Common part with ACPI 1.0
    uint32_t Length;          // Total length of this structure
    uint64_t XsdtAddress;     // Physical address of XSDT
    uint8_t ExtendedChecksum; // Checksum for the entire 2.0 structure
    uint8_t reserved[3];
};

// Generic ACPI System Description Table Header
struct ACPISDTHeader {
    char Signature[4];      // e.g., "FACP", "APIC", "HPET"
    uint32_t Length;        // Length of the table, including the header
    uint8_t Revision;
    uint8_t Checksum;       // To make sum of all bytes in table 0
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
};

// Fixed ACPI Description Table (FADT or FACP)
struct FADT {
    ACPISDTHeader Header;
    uint32_t FirmwareCtrl;      // Physical address of FACS
    uint32_t Dsdt;              // Physical address of DSDT

    // Field used in ACPI 1.0; no longer in ACPI 2.0+
    uint8_t  Reserved1;

    uint8_t  PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;     // System Control Interrupt
    uint32_t SMI_CommandPort;   // System Management Interrupt Command Port
    uint8_t  AcpiEnable;        // Value to write to SMI_CMD to enable ACPI
    uint8_t  AcpiDisable;       // Value to write to SMI_CMD to disable ACPI
    uint8_t  S4BIOS_REQ;        // Value to write to PM1a_CNT for S4BIOS state
    uint8_t  PSTATE_Control;    // Processor performance state control
    uint32_t PM1aEventBlock;    // Port address of Power Mgt 1a Event Reg Blk
    uint32_t PM1bEventBlock;    // Port address of Power Mgt 1b Event Reg Blk
    uint32_t PM1aControlBlock;  // Port address of Power Mgt 1a Control Reg Blk
    uint32_t PM1bControlBlock;  // Port address of Power Mgt 1b Control Reg Blk
    uint32_t PM2ControlBlock;   // Port address of Power Mgt 2 Control Reg Blk
    uint32_t PMTimerBlock;      // Port address of Power Mgt Timer Ctrl Reg Blk
    uint32_t GPE0Block;         // Port address of General Purpose Event 0 Reg Blk
    uint32_t GPE1Block;         // Port address of General Purpose Event 1 Reg Blk
    uint8_t  PM1EventLength;
    uint8_t  PM1ControlLength;
    uint8_t  PM2ControlLength;
    uint8_t  PMTimerLength;
    uint8_t  GPE0Length;
    uint8_t  GPE1Length;
    uint8_t  GPE1Base;
    uint8_t  CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t  DutyOffset;
    uint8_t  DutyWidth;
    uint8_t  DayAlarm;
    uint8_t  MonthAlarm;
    uint8_t  Century;

    // Reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t BootArchitectureFlags;

    uint8_t  Reserved2;
    uint32_t Flags;

    // Generic Address Structures (12 bytes each)
    // These describe registers in a generic way (address space, bit width, offset, access size)
    // For simplicity here, we just define them as byte arrays if we are not fully parsing them.
    // A full GenericAddressStructure would look like:
    // struct GenericAddressStructure {
    //   uint8_t AddressSpace;
    //   uint8_t BitWidth;
    //   uint8_t BitOffset;
    //   uint8_t AccessSize;
    //   uint64_t Address;
    // };
    uint8_t  ResetReg[12]; // GenericAddressStructure for RESET_REG

    uint8_t  ResetValue;   // Value to write to RESET_REG to reset
    uint8_t  Reserved3[3];

    // X versions of fields (64-bit addresses for ACPI 2.0+)
    // These are only valid if the FADT revision indicates ACPI 2.0+ and the fields are used.
    // The FADT length will be larger if these fields are present.
    // Accessing these without checking FADT.Header.Length can lead to out-of-bounds reads.
    uint64_t X_FirmwareControl;
    uint64_t X_Dsdt;
    uint8_t  X_PM1aEventBlock[12];
    uint8_t  X_PM1bEventBlock[12];
    uint8_t  X_PM1aControlBlock[12];
    uint8_t  X_PM1bControlBlock[12];
    uint8_t  X_PM2ControlBlock[12];
    uint8_t  X_PMTimerBlock[12];
    uint8_t  X_GPE0Block[12];
    uint8_t  X_GPE1Block[12];
    // uint8_t  X_SleepControlBlock[12]; // Added in ACPI 3.0
    // uint8_t  X_SleepStatusBlock[12];  // Added in ACPI 3.0
};

#pragma pack(pop) // Restore default packing


// --- Global Variables (declared as extern, defined in a .cpp file) ---
extern FADT* g_fadt; // Global pointer to the parsed FADT


// --- Constants for ACPI Shutdown ---
// These are common values, but the actual SLP_TYP for S5 should be read from DSDT _S5_ object.
// For a simplified approach, we use a common guess for PM1x_CNT register.
const uint16_t SLP_TYP_S5_PM1_CNT = (5 << 10); // Value for S5 sleep type (bits 10-12)
                                              // common for PM1a/b_CNT. (5, 6, or 7 are typical)
const uint16_t SLP_EN_PM1_CNT     = (1 << 13); // Sleep Enable bit (bit 13) for PM1a/b_CNT


// --- Function Declarations (to be implemented in a .cpp file) ---

/**
 * @brief Validates the checksum of an ACPI System Description Table.
 * @param tableHeader Pointer to the table's ACPISDTHeader.
 * @return True if checksum is valid, false otherwise.
 */
bool validate_acpi_sdt_checksum(ACPISDTHeader* tableHeader);

/**
 * @brief Finds the Root System Description Pointer (RSDP).
 * Searches EBDA and main BIOS area.
 * @return Pointer to RSDPDescriptor or RSDPDescriptor20 if found and valid, nullptr otherwise.
 *         The caller needs to check the Revision field to determine the version.
 */
void* find_rsdp();

/**
 * @brief Finds a specific System Description Table (e.g., FADT, APIC) by its signature.
 * @param rsdp_ptr Pointer to the RSDP (can be v1 or v2).
 * @param signature The 4-character signature of the table to find (e.g., "FACP").
 * @return Pointer to the ACPISDTHeader of the found table if valid, nullptr otherwise.
 */
ACPISDTHeader* find_sdt_from_rsdp(void* rsdp_ptr, const char* signature);

/**
 * @brief Initializes ACPI by finding the FADT and enabling ACPI mode.
 * Populates g_fadt.
 */
void acpi_init();

/**
 * @brief Attempts to power off the system using ACPI S5 soft-off state.
 * Requires acpi_init() to have been called successfully.
 * This function will not return if successful.
 */
void acpi_power_off();
void acpi_reboot();
void acpi_keyboard_reboot();

#endif // ACPI_H