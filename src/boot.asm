; boot.asm - Multiboot-compliant bootloader for 32-bit mode

section .multiboot
align 4
    ; Multiboot header
    MB_MAGIC equ 0x1BADB002          ; Magic number
    MB_FLAGS equ 0x00000003          ; Flags: align modules on page boundaries and provide memory map
    MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS) ; Checksum

    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .note.GNU-stack noalloc noexec nowrite progbits
; Add this section to prevent linker warnings about executable stack

section .data
align 4
mboot_info_ptr:
    dd 0                         ; Store multiboot info pointer here

section .bss
align 16
stack_bottom:
    resb 16384                   ; 16 KiB stack
stack_top:

section .text
global _start
global mboot_info_ptr            ; Export address of the storage for the pointer
extern kernel_main               ; C++ kernel entry point

; GDT Selectors (makes code more readable)
GDT_CODE_SELECTOR equ 0x08
GDT_DATA_SELECTOR equ 0x10

_start:
    ; Ensure interrupts are disabled on entry
    cli

    ; Save multiboot info pointer passed in ebx by GRUB
    mov [mboot_info_ptr], ebx

    ; Set up the stack
    mov esp, stack_top

    ; Load GDT (Global Descriptor Table) for 32-bit mode
    lgdt [gdt_pointer]

    ; Perform far jump to load CS and flush pipeline
    jmp GDT_CODE_SELECTOR:reload_segments

reload_segments:
    ; Reload data segment registers
    mov ax, GDT_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax                   ; Reload stack segment register too

    ; (Optional but harmless) Enable A20 line using "Fast A20 Gate"
    in al, 0x92
    test al, 2                   ; Check if bit 1 (A20 gate) is already set
    jnz a20_done                 ; Skip if already enabled
    or al, 2                     ; Set bit 1
    out 0x92, al
a20_done:

    ; Call the kernel main function, passing the address stored in mboot_info_ptr
    push dword [mboot_info_ptr]  ;
    call kernel_main

    ; Clean up stack (optional, but good practice if kernel could return)
    add esp, 4                   ; Remove the pointer pushed for kernel_main

    ; If the kernel returns, hang the CPU
hang:
    cli                          ; Disable interrupts (again, just in case)
    hlt                          ; Halt the CPU
    jmp hang                     ; Loop indefinitely

; --- GDT Definition ---
align 8
gdt_start:
    ; Null descriptor (required)
    dq 0x0000000000000000

    ; Code Segment Descriptor (Ring 0, Base 0, Limit 4G, 32-bit, Executable/Readable)
    ; Limit = 0xFFFF (16 bits), Base = 0 (24 bits), Access = 0x9A, Flags+Limit = 0xCF
    dq 0x00CF9A000000FFFF

    ; Data Segment Descriptor (Ring 0, Base 0, Limit 4G, 32-bit, Writable)
    ; Limit = 0xFFFF (16 bits), Base = 0 (24 bits), Access = 0x92, Flags+Limit = 0xCF
    dq 0x00CF92000000FFFF
gdt_end:

; GDT Pointer Structure (for lgdt instruction)
gdt_pointer:
    dw gdt_end - gdt_start - 1  ; Limit (Size of GDT - 1)
    dd gdt_start                ; Base Address of GDT