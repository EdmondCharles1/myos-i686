# =============================================================================
# Makefile - myos-i686 Build System
# =============================================================================

# -----------------------------------------------------------------------------
# Configuration des outils
# -----------------------------------------------------------------------------
CC      = i686-elf-gcc
AS      = nasm
LD      = i686-elf-ld

# Répertoires
KERNEL_SRC  = kernel/src
KERNEL_BUILD = kernel/build
KERNEL_ISO   = kernel/iso
DIST         = dist

# Flags du compilateur C
CFLAGS  = -std=c11 -ffreestanding -O2 -Wall -Wextra -nostdlib -I$(KERNEL_SRC)

# Flags de l'assembleur NASM
ASFLAGS = -f elf32

# Flags du linker
LDFLAGS = -T $(KERNEL_SRC)/linker.ld -nostdlib

# Fichiers sources
BOOT_ASM = boot.asm
KERNEL_C = $(KERNEL_SRC)/kernel.c

# Fichiers objets
BOOT_O   = $(KERNEL_BUILD)/boot.o
KERNEL_O = $(KERNEL_BUILD)/kernel.o
OBJS     = $(BOOT_O) $(KERNEL_O)

# Chemin complet de libgcc
LIBGCC = $(shell i686-elf-gcc -print-libgcc-file-name)

# Nom du binaire final
KERNEL_BIN = $(KERNEL_BUILD)/myos.bin

# Nom de l'ISO
ISO_NAME = myos.iso
ISO_FILE = $(DIST)/$(ISO_NAME)

# -----------------------------------------------------------------------------
# Règles de compilation
# -----------------------------------------------------------------------------

# Règle par défaut : compile tout
all: dirs $(KERNEL_BIN)

# Créer les répertoires nécessaires
dirs:
	@mkdir -p $(KERNEL_BUILD)
	@mkdir -p $(DIST)

# Assemblage de boot.asm
$(BOOT_O): $(BOOT_ASM)
	@echo "[AS] $(BOOT_ASM) -> $(BOOT_O)"
	@$(AS) $(ASFLAGS) $(BOOT_ASM) -o $(BOOT_O)

# Compilation de kernel.c
$(KERNEL_O): $(KERNEL_C)
	@echo "[CC] $(KERNEL_C) -> $(KERNEL_O)"
	@$(CC) $(CFLAGS) -c $(KERNEL_C) -o $(KERNEL_O)

# Linkage final (avec chemin complet de libgcc)
$(KERNEL_BIN): $(OBJS)
	@echo "[LD] Linkage -> $(KERNEL_BIN)"
	@$(LD) $(LDFLAGS) $(OBJS) -o $(KERNEL_BIN) $(LIBGCC)
	@echo ""
	@echo "✅ Compilation réussie !"
	@echo "Fichier généré : $(KERNEL_BIN)"
	@file $(KERNEL_BIN)

# Vérification Multiboot
check: $(KERNEL_BIN)
	@echo "Vérification Multiboot..."
	@if command -v grub-file >/dev/null 2>&1; then \
		grub-file --is-x86-multiboot $(KERNEL_BIN) && echo "✅ Multiboot valide" || echo "❌ Multiboot invalide"; \
	else \
		echo "⚠️  grub-file non installé, vérification manuelle..."; \
		hexdump -C $(KERNEL_BIN) | head -5 | grep -q "02 b0 ad 1b" && echo "✅ Magic Multiboot trouvé" || echo "❌ Magic Multiboot absent"; \
	fi

# Nettoyage des fichiers de build
clean:
	@echo "Nettoyage des fichiers de build..."
	@rm -rf $(KERNEL_BUILD)
	@echo "✅ Nettoyage terminé"

# Nettoyage complet (build + dist)
distclean: clean
	@echo "Nettoyage complet..."
	@rm -rf $(DIST)
	@echo "✅ Nettoyage complet terminé"

# Recompilation complète
rebuild: clean all

# Informations sur le projet
info:
	@echo "=== myos-i686 Build Information ==="
	@echo "Compilateur : $(CC)"
	@echo "Assembleur  : $(AS)"
	@echo "Linker      : $(LD)"
	@echo "Kernel bin  : $(KERNEL_BIN)"
	@echo "libgcc      : $(LIBGCC)"
	@echo ""
	@echo "=== Répertoires ==="
	@echo "Sources     : $(KERNEL_SRC)"
	@echo "Build       : $(KERNEL_BUILD)"
	@echo "Distribution: $(DIST)"
	@echo ""
	@echo "=== Fichiers sources ==="
	@ls -lh $(BOOT_ASM) $(KERNEL_C) $(KERNEL_SRC)/linker.ld 2>/dev/null || true

# Affiche la structure du projet
tree:
	@tree -L 3 -I 'build|iso|dist' .

# Debug: affiche les variables
debug:
	@echo "LIBGCC = $(LIBGCC)"
	@echo "OBJS = $(OBJS)"
	@echo "LDFLAGS = $(LDFLAGS)"

.PHONY: all dirs clean distclean rebuild check info tree debug
