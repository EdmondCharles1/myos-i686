# =============================================================================
# Makefile - myos-i686 Build System (avec SSP)
# =============================================================================

CC      = i686-elf-gcc
AS      = nasm

# Répertoires
KERNEL_SRC  = kernel/src
KERNEL_BUILD = kernel/build
KERNEL_ISO   = kernel/iso
DIST         = dist

# Flags du compilateur C
CFLAGS  = -std=c11 -ffreestanding -O2 -Wall -Wextra -I$(KERNEL_SRC)
# Activer le Stack Smashing Protector
CFLAGS += -fstack-protector-all

# Flags de l'assembleur NASM
ASFLAGS = -f elf32

# Flags du linker
LDFLAGS = -T $(KERNEL_SRC)/linker.ld -ffreestanding -nostdlib -lgcc

# Fichiers sources
BOOT_ASM = boot.asm
KERNEL_C = $(KERNEL_SRC)/kernel.c
PRINTF_C = $(KERNEL_SRC)/printf.c
STACK_C  = $(KERNEL_SRC)/stack_protector.c

# Fichiers objets
BOOT_O   = $(KERNEL_BUILD)/boot.o
KERNEL_O = $(KERNEL_BUILD)/kernel.o
PRINTF_O = $(KERNEL_BUILD)/printf.o
STACK_O  = $(KERNEL_BUILD)/stack_protector.o
OBJS     = $(BOOT_O) $(KERNEL_O) $(PRINTF_O) $(STACK_O)

# Binaires
KERNEL_BIN = $(KERNEL_BUILD)/myos.bin
ISO_FILE   = $(DIST)/myos.iso

# -----------------------------------------------------------------------------
# Règles de compilation
# -----------------------------------------------------------------------------

all: dirs $(KERNEL_BIN)

dirs:
	@mkdir -p $(KERNEL_BUILD)
	@mkdir -p $(DIST)
	@mkdir -p $(KERNEL_ISO)/boot/grub

$(BOOT_O): $(BOOT_ASM)
	@echo "[AS] $(BOOT_ASM) -> $(BOOT_O)"
	@$(AS) $(ASFLAGS) $(BOOT_ASM) -o $(BOOT_O)

$(KERNEL_O): $(KERNEL_C) $(KERNEL_SRC)/printf.h $(KERNEL_SRC)/stack_protector.h
	@echo "[CC] $(KERNEL_C) -> $(KERNEL_O)"
	@$(CC) $(CFLAGS) -c $(KERNEL_C) -o $(KERNEL_O)

$(PRINTF_O): $(PRINTF_C) $(KERNEL_SRC)/printf.h
	@echo "[CC] $(PRINTF_C) -> $(PRINTF_O)"
	@$(CC) $(CFLAGS) -c $(PRINTF_C) -o $(PRINTF_O)

$(STACK_O): $(STACK_C) $(KERNEL_SRC)/stack_protector.h
	@echo "[CC] $(STACK_C) -> $(STACK_O)"
	@$(CC) $(CFLAGS) -c $(STACK_C) -o $(STACK_O)

$(KERNEL_BIN): $(OBJS)
	@echo "[LD] Linkage -> $(KERNEL_BIN)"
	@$(CC) $(LDFLAGS) $(OBJS) -o $(KERNEL_BIN)
	@echo ""
	@echo "✅ Compilation réussie !"
	@echo "Fichier généré : $(KERNEL_BIN)"
	@file $(KERNEL_BIN)

# -----------------------------------------------------------------------------
# ISO
# -----------------------------------------------------------------------------

iso: $(KERNEL_BIN)
	@echo ""
	@echo "📀 Création de l'image ISO..."
	@mkdir -p $(KERNEL_ISO)/boot/grub
	@cp $(KERNEL_BIN) $(KERNEL_ISO)/boot/myos.bin
	@echo "✓ Kernel copié"
	@if [ ! -f $(KERNEL_ISO)/boot/grub/grub.cfg ]; then \
		echo "❌ grub.cfg manquant !"; exit 1; \
	fi
	@grub-mkrescue -o $(ISO_FILE) $(KERNEL_ISO) 2>/dev/null
	@echo ""
	@echo "✅ ISO créée : $(ISO_FILE)"
	@ls -lh $(ISO_FILE)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

check: $(KERNEL_BIN)
	@echo "Vérification Multiboot..."
	@grub-file --is-x86-multiboot $(KERNEL_BIN) && echo "✅ Multiboot valide" || echo "❌ Multiboot invalide"

run: $(KERNEL_BIN)
	@echo "🚀 Lancement du kernel..."
	@qemu-system-i386 -kernel $(KERNEL_BIN)

run-iso: iso
	@echo "🚀 Lancement de l'ISO..."
	@qemu-system-i386 -cdrom $(ISO_FILE)

# -----------------------------------------------------------------------------
# Nettoyage
# -----------------------------------------------------------------------------

clean:
	@echo "🧹 Nettoyage..."
	@rm -rf $(KERNEL_BUILD)

clean-iso:
	@rm -rf $(KERNEL_ISO)/boot/myos.bin
	@rm -f $(ISO_FILE)

distclean: clean clean-iso
	@rm -rf $(DIST)

rebuild: clean all

rebuild-iso: clean-iso iso

# -----------------------------------------------------------------------------
# Info
# -----------------------------------------------------------------------------

info:
	@echo "=== myos-i686 Build Information ==="
	@echo "Compilateur : $(CC)"
	@echo "Flags SSP   : -fstack-protector-all"
	@echo "Kernel bin  : $(KERNEL_BIN)"
	@echo "ISO file    : $(ISO_FILE)"

.PHONY: all dirs clean clean-iso distclean rebuild rebuild-iso check info iso run run-iso
