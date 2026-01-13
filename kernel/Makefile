# Toolchain
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld

# Flags
CFLAGS = -std=c11 -ffreestanding -O2 -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -T src/linker.ld -nostdlib -lgcc

# Directories
SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso

# Files
KERNEL_BIN = $(BUILD_DIR)/myos.bin
ISO_FILE = $(BUILD_DIR)/myos.iso

# Source files
ASM_SRC = $(SRC_DIR)/boot.asm
C_SRC = $(SRC_DIR)/kernel.c

# Object files
ASM_OBJ = $(BUILD_DIR)/boot.o
C_OBJ = $(BUILD_DIR)/kernel.o

all: $(KERNEL_BIN) $(ISO_FILE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ISO_DIR):
	mkdir -p $(ISO_DIR)/boot/grub

$(ASM_OBJ): $(ASM_SRC) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(ASM_SRC) -o $(ASM_OBJ)

$(C_OBJ): $(C_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $(C_SRC) -o $(C_OBJ)

$(KERNEL_BIN): $(ASM_OBJ) $(C_OBJ)
	$(LD) $(LDFLAGS) $(ASM_OBJ) $(C_OBJ) -o $(KERNEL_BIN)

$(ISO_FILE): $(KERNEL_BIN) | $(ISO_DIR)
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/myos.bin
	echo 'menuentry "MyOS" { multiboot /boot/myos.bin }' > $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)

run: $(ISO_FILE)
	qemu-system-i386 -cdrom $(ISO_FILE)

clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR)

.PHONY: all run clean
