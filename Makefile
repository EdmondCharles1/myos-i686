# =============================================================================
# Makefile - myos-i686 Build System
# Avec: SSP, Interruptions, Timer, Process Management
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
LDFLAGS = -T $(KERNEL_SRC)/linker.ld -ffreestanding -nostdlib

# Fichiers sources assembleur
BOOT_ASM = boot.asm
ISR_ASM  = $(KERNEL_SRC)/isr_asm.asm

# Fichiers sources C
C_SOURCES = \
	$(KERNEL_SRC)/kernel.c \
	$(KERNEL_SRC)/printf.c \
	$(KERNEL_SRC)/stack_protector.c \
	$(KERNEL_SRC)/idt.c \
	$(KERNEL_SRC)/isr.c \
	$(KERNEL_SRC)/irq.c \
	$(KERNEL_SRC)/pic.c \
	$(KERNEL_SRC)/timer.c \
	$(KERNEL_SRC)/process.c

# Fichiers objets (générés automatiquement)
BOOT_O    = $(KERNEL_BUILD)/boot.o
ISR_ASM_O = $(KERNEL_BUILD)/isr_asm.o
C_OBJS    = $(patsubst $(KERNEL_SRC)/%.c, $(KERNEL_BUILD)/%.o, $(C_SOURCES))

# Tous les objets
OBJS = $(BOOT_O) $(ISR_ASM_O) $(C_OBJS)

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

# Assemblage de boot.asm
$(BOOT_O): $(BOOT_ASM)
	@echo "[AS] $(BOOT_ASM) -> $(BOOT_O)"
	@$(AS) $(ASFLAGS) $(BOOT_ASM) -o $(BOOT_O)

# Assemblage de isr_asm.asm
$(ISR_ASM_O): $(ISR_ASM)
	@echo "[AS] $(ISR_ASM) -> $(ISR_ASM_O)"
	@$(AS) $(ASFLAGS) $(ISR_ASM) -o $(ISR_ASM_O)

# Règle générique pour compiler les fichiers C
$(KERNEL_BUILD)/%.o: $(KERNEL_SRC)/%.c
	@echo "[CC] $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

# Linkage final
$(KERNEL_BIN): $(OBJS)
	@echo "[LD] Linkage -> $(KERNEL_BIN)"
	@$(CC) $(LDFLAGS) $(OBJS) -o $(KERNEL_BIN) -lgcc
	@echo ""
	@echo "✅ Compilation réussie !"
	@echo "Fichier généré : $(KERNEL_BIN)"
	@file $(KERNEL_BIN)
	@echo ""
	@echo "📦 Modules compilés :"
	@echo "  ✓ IDT (Interrupt Descriptor Table)"
	@echo "  ✓ ISR (CPU Exceptions 0-31)"
	@echo "  ✓ IRQ (Hardware Interrupts 32-47)"
	@echo "  ✓ PIC (8259 Interrupt Controller)"
	@echo "  ✓ Timer (PIT 8253 @ 100 Hz)"
	@echo "  ✓ Process Manager (PCB + Scheduler)"
	@echo "  ✓ Printf (formatted output)"
	@echo "  ✓ Stack Protector (SSP)"

# -----------------------------------------------------------------------------
# ISO
# -----------------------------------------------------------------------------

iso: $(KERNEL_BIN)
	@echo ""
	@echo "📀 Création de l'image ISO..."
	@mkdir -p $(KERNEL_ISO)/boot/grub
	@cp $(KERNEL_BIN) $(KERNEL_ISO)/boot/myos.bin
	@echo "✓ Kernel copié dans $(KERNEL_ISO)/boot/"
	@if [ ! -f $(KERNEL_ISO)/boot/grub/grub.cfg ]; then \
		echo "❌ Erreur: grub.cfg manquant !"; \
		echo "Créez le fichier: $(KERNEL_ISO)/boot/grub/grub.cfg"; \
		exit 1; \
	fi
	@echo "✓ Configuration GRUB trouvée"
	@grub-mkrescue -o $(ISO_FILE) $(KERNEL_ISO) 2>/dev/null || \
		(echo "❌ Erreur lors de la création de l'ISO" && exit 1)
	@echo ""
	@echo "✅ ISO créée avec succès !"
	@echo "Fichier : $(ISO_FILE)"
	@ls -lh $(ISO_FILE)

# -----------------------------------------------------------------------------
# Tests et exécution
# -----------------------------------------------------------------------------

check: $(KERNEL_BIN)
	@echo "🔍 Vérification Multiboot..."
	@grub-file --is-x86-multiboot $(KERNEL_BIN) && \
		echo "✅ Header Multiboot valide" || \
		echo "❌ Header Multiboot invalide"
	@echo ""
	@echo "📊 Informations du binaire :"
	@file $(KERNEL_BIN)
	@echo ""
	@echo "📏 Taille du kernel :"
	@ls -lh $(KERNEL_BIN)
	@echo ""
	@echo "🔢 Symboles principaux :"
	@i686-elf-nm $(KERNEL_BIN) | grep -E "(kernel_main|_start|timer_init|idt_init|process_init)" || true

run: $(KERNEL_BIN)
	@echo "🚀 Lancement du kernel dans QEMU (mode direct)..."
	@echo "   Appuyez sur Ctrl+C pour quitter"
	@echo ""
	@qemu-system-i386 -kernel $(KERNEL_BIN)

run-iso: iso
	@echo "🚀 Lancement de l'ISO dans QEMU (boot GRUB)..."
	@echo "   Appuyez sur Ctrl+C pour quitter"
	@echo ""
	@qemu-system-i386 -cdrom $(ISO_FILE)

run-debug: $(KERNEL_BIN)
	@echo "🐛 Lancement en mode debug..."
	@echo "   Logs des interruptions activés"
	@echo "   Appuyez sur Ctrl+C pour quitter"
	@echo ""
	@qemu-system-i386 -kernel $(KERNEL_BIN) -d int,cpu_reset -no-reboot

# -----------------------------------------------------------------------------
# Nettoyage
# -----------------------------------------------------------------------------

clean:
	@echo "🧹 Nettoyage des fichiers de build..."
	@rm -rf $(KERNEL_BUILD)
	@echo "✅ Fichiers .o supprimés"

clean-iso:
	@echo "🧹 Nettoyage de l'ISO..."
	@rm -rf $(KERNEL_ISO)/boot/myos.bin
	@rm -f $(ISO_FILE)
	@echo "✅ ISO supprimée"

distclean: clean clean-iso
	@echo "🧹 Nettoyage complet (build + dist)..."
	@rm -rf $(DIST)
	@echo "✅ Nettoyage complet terminé"

rebuild: clean all
	@echo ""
	@echo "🔄 Recompilation complète terminée"

rebuild-iso: clean-iso iso
	@echo ""
	@echo "🔄 Reconstruction ISO terminée"

# -----------------------------------------------------------------------------
# Informations et aide
# -----------------------------------------------------------------------------

info:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║         myos-i686 - Build Information                      ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ Compilateur    : $(CC)                               ║"
	@echo "║ Assembleur     : $(AS)                                    ║"
	@echo "║ Flags SSP      : -fstack-protector-all                     ║"
	@echo "║ Optimisation   : -O2                                       ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ Kernel binaire : $(KERNEL_BIN)                    ║"
	@echo "║ Image ISO      : $(DIST)/myos.iso                          ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ Modules système compilés :                                 ║"
	@echo "║   • Boot & Multiboot header                                ║"
	@echo "║   • IDT (256 entrées d'interruptions)                      ║"
	@echo "║   • ISR (32 exceptions CPU)                                ║"
	@echo "║   • IRQ (16 interruptions hardware)                        ║"
	@echo "║   • PIC 8259 (remappage IRQ 0-15 → INT 32-47)              ║"
	@echo "║   • Timer PIT 8253 (100 Hz / 10ms period)                  ║"
	@echo "║   • Process Manager (PCB, états, création/terminaison)     ║"
	@echo "║   • Printf (formatted output)                              ║"
	@echo "║   • Stack Smashing Protector                               ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@if [ -d $(KERNEL_BUILD) ] && [ "$$(ls -A $(KERNEL_BUILD) 2>/dev/null)" ]; then \
		echo "📦 Fichiers objets compilés :"; \
		ls -lh $(KERNEL_BUILD)/*.o 2>/dev/null | awk '{print "   " $$9 " (" $$5 ")"}'; \
		echo ""; \
		echo "📈 Taille totale des objets :"; \
		du -sh $(KERNEL_BUILD) 2>/dev/null; \
	else \
		echo "⚠️  Aucun fichier compilé (lancez 'make')"; \
	fi
	@echo ""
	@if [ -f $(KERNEL_BIN) ]; then \
		echo "🎯 Kernel final :"; \
		ls -lh $(KERNEL_BIN); \
	fi

help:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║              myos-i686 - Commandes Make                    ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ COMPILATION                                                ║"
	@echo "║   make           Compile le kernel                         ║"
	@echo "║   make iso       Crée l'image ISO bootable                 ║"
	@echo "║   make rebuild   Nettoie et recompile                      ║"
	@echo "║   make check     Vérifie le binaire (Multiboot, etc.)      ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ EXÉCUTION                                                  ║"
	@echo "║   make run       Lance le kernel (direct)                  ║"
	@echo "║   make run-iso   Lance l'ISO (avec GRUB)                   ║"
	@echo "║   make run-debug Lance avec logs de debug                  ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ NETTOYAGE                                                  ║"
	@echo "║   make clean     Supprime les fichiers .o                  ║"
	@echo "║   make clean-iso Supprime l'ISO                            ║"
	@echo "║   make distclean Nettoyage complet                         ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ INFORMATIONS                                               ║"
	@echo "║   make info      Affiche les infos du projet               ║"
	@echo "║   make help      Affiche cette aide                        ║"
	@echo "║   make list      Liste les fichiers sources                ║"
	@echo "╚════════════════════════════════════════════════════════════╝"

list:
	@echo "📁 Fichiers sources du projet :"
	@echo ""
	@echo "📄 Assembleur :"
	@echo "   • $(BOOT_ASM)"
	@echo "   • $(ISR_ASM)"
	@echo ""
	@echo "📄 C ($(words $(C_SOURCES)) fichiers) :"
	@for file in $(C_SOURCES); do \
		echo "   • $$file"; \
	done
	@echo ""
	@echo "🔧 Headers :"
	@ls $(KERNEL_SRC)/*.h 2>/dev/null | while read file; do \
		echo "   • $$file"; \
	done || echo "   (aucun)"
	@echo ""
	@echo "📊 Statistiques :"
	@echo "   Lignes de code C :"
	@wc -l $(C_SOURCES) 2>/dev/null | tail -1 | awk '{print "     " $$1 " lignes"}'
	@echo "   Lignes de code ASM :"
	@wc -l $(BOOT_ASM) $(ISR_ASM) 2>/dev/null | tail -1 | awk '{print "     " $$1 " lignes"}'

.PHONY: all dirs clean clean-iso distclean rebuild rebuild-iso check \
        info help iso run run-iso run-debug list
