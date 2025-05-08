 #!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# --- Configuration ---
# Compiler and Linker Flags
# -m32: Target 32-bit i386
# -ffreestanding: Kernel environment, no standard library hosted assumptions
# -fno-exceptions: Disable C++ exceptions
# -fno-rtti: Disable Run-Time Type Information
# -O2: Optimization level 2
# -Wall: Enable all warnings (good practice)
# -Wextra: Enable extra warnings (good practice)
# -std=c++11: Or c++14, c++17, c++20 depending on features used (e.g., noexcept, alignas)
# -Iinclude: Tell compiler where to find headers (e.g., include/consts.h)
# -Wno-unused-parameter: Temporarily suppress unused param warnings if needed (e.g. for 'signature' if it persists)
# -Wno-unused-variable: Temporarily suppress unused var warnings if needed
CFLAGS="-m32 -ffreestanding -fno-exceptions -fno-rtti -O2 -Wall -Wextra -std=c++11 -Iinclude"
LINKER_SCRIPT="linker.ld" # Define the linker script name

# Source files (add all your .cpp files here)
CPP_SOURCES="kernel.cpp consts.cpp memorys.cpp screens.cpp io.cpp acpi.cpp"
ASM_SOURCES="boot.asm"

# Object files will be placed in build/
# Create an array to hold object file names
declare -a OBJECT_FILES_ARRAY

# Output names
KERNEL_BIN="build/kernel.bin"
ISO_NAME="build/cinemintos.iso"

# --- Dependencies ---
echo "Checking/Installing dependencies..."
# Check if sudo is needed or if user is root
if [ "$(id -u)" -ne 0 ]; then
    SUDO_CMD="sudo"
else
    SUDO_CMD=""
fi
# Consider only running apt-get if needed or less frequently
# $SUDO_CMD apt-get update
$SUDO_CMD apt-get install -y g++ g++-multilib gcc-multilib nasm qemu-system-x86 grub-pc-bin xorriso mtools

# --- Build ---
echo "Creating build directory..."
mkdir -p build
mkdir -p build/iso/boot/grub # Ensure full ISO path exists

# Compile the assembly bootloader(s)
echo "Compiling assembly source files..."
for src_file in $ASM_SOURCES; do
   obj_file="build/${src_file%.asm}.o"
   echo "  Compiling $src_file -> $obj_file"
   nasm -f elf32 "$src_file" -o "$obj_file"
   OBJECT_FILES_ARRAY+=("$obj_file") # Add to the list
done

# Compile C++ source files
echo "Compiling C++ source files..."
for src_file in $CPP_SOURCES; do
    obj_file="build/${src_file%.cpp}.o"
    echo "  Compiling $src_file -> $obj_file"
    g++ $CFLAGS -c "$src_file" -o "$obj_file"
    OBJECT_FILES_ARRAY+=("$obj_file") # Add to the list
done

# Link the kernel using g++ as the driver
echo "Linking kernel..."
# Use g++ for linking: passes correct -m32 flags, knows where libgcc is.
# -nostdlib: Prevents linking standard C libraries and startup files.
# -lgcc: Explicitly link libgcc to resolve compiler intrinsics like __udivmoddi4.
# Note: CFLAGS is passed to g++ again during link stage to ensure -m32 is active
# and to make the linker aware of the target architecture for library paths.
echo "  Linker command: g++ $CFLAGS -T $LINKER_SCRIPT -o $KERNEL_BIN ${OBJECT_FILES_ARRAY[@]} -nostdlib -lgcc"
g++ $CFLAGS -T $LINKER_SCRIPT -o $KERNEL_BIN "${OBJECT_FILES_ARRAY[@]}" -nostdlib -lgcc

# Check if kernel.bin exists and has size greater than 0
if [ ! -s "$KERNEL_BIN" ]; then
    echo "Error: $KERNEL_BIN is empty or does not exist after linking."
    exit 1
else
    echo "Kernel linked successfully: $KERNEL_BIN"
fi

# Create a bootable ISO
echo "Creating bootable ISO ($ISO_NAME)..."
cp "$KERNEL_BIN" build/iso/boot/kernel.bin # Ensure correct kernel name for GRUB

# Create grub.cfg
cat > build/iso/boot/grub/grub.cfg << EOF
set timeout=0
set default=0

menuentry "Cinemint OS" {
    multiboot /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o "$ISO_NAME" build/iso

echo "Build complete. ISO created: $ISO_NAME"

# --- Run (optional) ---
echo "Running Cinemint OS in QEMU..."
qemu-system-i386 -cdrom "$ISO_NAME"