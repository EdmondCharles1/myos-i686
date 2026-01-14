# =============================================================================
# Makefile - myos-i686 Build System (avec printf)
# =============================================================================

CC      = i686-elf-gcc
AS      = nasm

# Répertoires
KERNEL_SRC  = kernel/src
KERNEL_BUILD = kernel/build
DIST         = dist

# Flags
CFLAGS  = -std=c11 -ffreestanding -O2 -Wall -Wextra -I$(KERNEL_SRC)
ASFLAGS = -f elf32
LDFLAGS = -T $(KERNEL_SRC)/linker.ld -ffreestanding -nostdlib -lgcc

# Fichiers sources
BOOT_ASM = boot.asm
KERNEL_C = $(KERNEL_SRC)/kernel.c
PRINTF_C = $(KERNEL_SRC)/printf.c

# Fichiers objets
BOOT_O   = $(KERNEL_BUILD)/boot.o
KERNEL_O = $(KERNEL_BUILD)/kernel.o
PRINTF_O = $(KERNEL_BUILD)/printf.o
OBJS     = $(BOOT_O) $(KERNEL_O) $(PRINTF_O)

# Binaire final
KERNEL_BIN = $(KERNEL_BUILD)/myos.bin

# -----------------------------------------------------------------------------
# Règles de compilation
# -----------------------------------------------------------------------------

all: dirs $(KERNEL_BIN)

dirs:
	@mkdir -p $(KERNEL_BUILD)
	@mkdir -p $(DIST)

$(BOOT_O): $(BOOT_ASM)
	@echo "[AS] $(BOOT_ASM) -> $(BOOT_O)"
	@$(AS) $(ASFLAGS) $(BOOT_ASM) -o $(BOOT_O)

$(KERNEL_O): $(KERNEL_C) $(KERNEL_SRC)/printf.h
	@echo "[CC] $(KERNEL_C) -> $(KERNEL_O)"
	@$(CC) $(CFLAGS) -c $(KERNEL_C) -o $(KERNEL_O)

$(PRINTF_O): $(PRINTF_C) $(KERNEL_SRC)/printf.h
	@echo "[CC] $(PRINTF_C) -> $(PRINTF_O)"
	@$(CC) $(CFLAGS) -c $(PRINTF_C) -o $(PRINTF_O)

$(KERNEL_BIN): $(OBJS)
	@echo "[LD] Linkage -> $(KERNEL_BIN)"
	@$(CC) $(LDFLAGS) $(OBJS) -o $(KERNEL_BIN)
	@echo ""
	@echo "✅ Compilation réussie !"
	@echo "Fichier généré : $(KERNEL_BIN)"
	@file $(KERNEL_BIN)

check: $(KERNEL_BIN)
	@echo "Vérification Multiboot..."
	@grub-file --is-x86-multiboot $(KERNEL_BIN) && echo "✅ Multiboot valide" || echo "❌ Multiboot invalide"

clean:
	@echo "Nettoyage..."
	@rm -rf $(KERNEL_BUILD)
	@echo "✅ Nettoyage terminé"

distclean: clean
	@rm -rf $(DIST)

rebuild: clean all

info:
	@echo "=== myos-i686 Build Information ==="
	@echo "Compilateur : $(CC)"
	@echo "Assembleur  : $(AS)"
	@echo "Kernel bin  : $(KERNEL_BIN)"
	@echo "Sources     : kernel.c, printf.c"

.PHONY: all dirs clean distclean rebuild check info
