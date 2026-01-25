# =============================================================================
# Makefile - myos-i686 Build System
# Parties 1-7 : Boot, VGA, Printf, SSP, IDT, ISR, IRQ, PIC, Timer, 
#               Process, Scheduler, Keyboard, Shell
# =============================================================================

CC      = i686-elf-gcc
AS      = nasm

# Répertoires
KERNEL_SRC   = kernel/src
KERNEL_BUILD = kernel/build
KERNEL_ISO   = kernel/iso
DIST         = dist

# Flags du compilateur C
CFLAGS  = -std=c11 -ffreestanding -O2 -Wall -Wextra -I$(KERNEL_SRC)
CFLAGS += -fstack-protector-all

# Flags de l'assembleur NASM
ASFLAGS = -f elf32

# Flags du linker
LDFLAGS = -T $(KERNEL_SRC)/linker.ld -ffreestanding -nostdlib

# Fichiers sources assembleur
BOOT_ASM = boot.asm
ISR_ASM  = $(KERNEL_SRC)/isr_asm.asm

# Fichiers sources C - Vérification d'existence conditionnelle
C_SOURCES = \
	$(KERNEL_SRC)/kernel.c \
	$(KERNEL_SRC)/printf.c

# Ajouter stack_protector.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/stack_protector.c),)
C_SOURCES += $(KERNEL_SRC)/stack_protector.c
endif

# Ajouter idt.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/idt.c),)
C_SOURCES += $(KERNEL_SRC)/idt.c
endif

# Ajouter isr.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/isr.c),)
C_SOURCES += $(KERNEL_SRC)/isr.c
endif

# Ajouter irq.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/irq.c),)
C_SOURCES += $(KERNEL_SRC)/irq.c
endif

# Ajouter pic.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/pic.c),)
C_SOURCES += $(KERNEL_SRC)/pic.c
endif

# Ajouter timer.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/timer.c),)
C_SOURCES += $(KERNEL_SRC)/timer.c
endif

# Ajouter process.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/process.c),)
C_SOURCES += $(KERNEL_SRC)/process.c
endif

# Ajouter scheduler.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/scheduler.c),)
C_SOURCES += $(KERNEL_SRC)/scheduler.c
endif

# Ajouter keyboard.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/keyboard.c),)
C_SOURCES += $(KERNEL_SRC)/keyboard.c
endif

# Ajouter shell.c si existe
ifneq ($(wildcard $(KERNEL_SRC)/shell.c),)
C_SOURCES += $(KERNEL_SRC)/shell.c
endif

# Fichiers objets
BOOT_O    = $(KERNEL_BUILD)/boot.o
C_OBJS    = $(patsubst $(KERNEL_SRC)/%.c, $(KERNEL_BUILD)/%.o, $(C_SOURCES))

# Ajouter isr_asm.o seulement si isr_asm.asm existe
OBJS = $(BOOT_O)
ifneq ($(wildcard $(ISR_ASM)),)
OBJS += $(KERNEL_BUILD)/isr_asm.o
endif
OBJS += $(C_OBJS)

# Binaires
KERNEL_BIN = $(KERNEL_BUILD)/myos.bin
ISO_FILE   = $(DIST)/myos.iso

# =============================================================================
# Règles de compilation
# =============================================================================

all: dirs $(KERNEL_BIN)

dirs:
	@mkdir -p $(KERNEL_BUILD)
	@mkdir -p $(DIST)
	@mkdir -p $(KERNEL_ISO)/boot/grub

# Assemblage de boot.asm
$(BOOT_O): $(BOOT_ASM)
	@echo "[AS] $(BOOT_ASM) -> $(BOOT_O)"
	@$(AS) $(ASFLAGS) $(BOOT_ASM) -o $(BOOT_O)

# Assemblage de isr_asm.asm (si existe)
$(KERNEL_BUILD)/isr_asm.o: $(ISR_ASM)
	@echo "[AS] $(ISR_ASM) -> $@"
	@$(AS) $(ASFLAGS) $(ISR_ASM) -o $@

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
	@file $(KERNEL_BIN) 2>/dev/null || true
	@echo ""
	@echo "📦 Modules détectés et compilés :"
	@test -f $(KERNEL_SRC)/printf.c && echo "  ✓ Printf (formatted output)" || true
	@test -f $(KERNEL_SRC)/stack_protector.c && echo "  ✓ Stack Protector (SSP)" || true
	@test -f $(KERNEL_SRC)/idt.c && echo "  ✓ IDT (Interrupt Descriptor Table)" || true
	@test -f $(KERNEL_SRC)/isr.c && echo "  ✓ ISR (CPU Exceptions 0-31)" || true
	@test -f $(KERNEL_SRC)/irq.c && echo "  ✓ IRQ (Hardware Interrupts 32-47)" || true
	@test -f $(KERNEL_SRC)/pic.c && echo "  ✓ PIC (8259 Interrupt Controller)" || true
	@test -f $(KERNEL_SRC)/timer.c && echo "  ✓ Timer (PIT 8253 @ 100 Hz)" || true
	@test -f $(KERNEL_SRC)/process.c && echo "  ✓ Process Manager (PCB + 32 slots)" || true
	@test -f $(KERNEL_SRC)/scheduler.c && echo "  ✓ Scheduler (FCFS + Round Robin)" || true
	@test -f $(KERNEL_SRC)/keyboard.c && echo "  ✓ Keyboard Driver (PS/2 IRQ1)" || true
	@test -f $(KERNEL_SRC)/shell.c && echo "  ✓ Mini-Shell (10 commandes)" || true
	@echo ""

# =============================================================================
# ISO
# =============================================================================

iso: $(KERNEL_BIN)
	@echo ""
	@echo "📀 Création de l'image ISO..."
	@mkdir -p $(KERNEL_ISO)/boot/grub
	@cp $(KERNEL_BIN) $(KERNEL_ISO)/boot/myos.bin
	@echo "✓ Kernel copié dans $(KERNEL_ISO)/boot/"
	@if [ ! -f $(KERNEL_ISO)/boot/grub/grub.cfg ]; then \
		echo "⚠️  grub.cfg manquant, création automatique..."; \
		echo "set timeout=0" > $(KERNEL_ISO)/boot/grub/grub.cfg; \
		echo "set default=0" >> $(KERNEL_ISO)/boot/grub/grub.cfg; \
		echo "" >> $(KERNEL_ISO)/boot/grub/grub.cfg; \
		echo "menuentry \"myos-i686\" {" >> $(KERNEL_ISO)/boot/grub/grub.cfg; \
		echo "    multiboot /boot/myos.bin" >> $(KERNEL_ISO)/boot/grub/grub.cfg; \
		echo "    boot" >> $(KERNEL_ISO)/boot/grub/grub.cfg; \
		echo "}" >> $(KERNEL_ISO)/boot/grub/grub.cfg; \
		echo "✓ grub.cfg créé automatiquement"; \
	fi
	@echo "✓ Configuration GRUB OK"
	@grub-mkrescue -o $(ISO_FILE) $(KERNEL_ISO) 2>/dev/null || \
		(echo "❌ Erreur lors de la création de l'ISO" && exit 1)
	@echo ""
	@echo "✅ ISO créée avec succès !"
	@echo "Fichier : $(ISO_FILE)"
	@ls -lh $(ISO_FILE) 2>/dev/null || true

# =============================================================================
# Tests et exécution
# =============================================================================

check: $(KERNEL_BIN)
	@echo "🔍 Vérification Multiboot..."
	@grub-file --is-x86-multiboot $(KERNEL_BIN) 2>/dev/null && \
		echo "✅ Header Multiboot valide" || \
		echo "❌ Header Multiboot invalide"
	@echo ""
	@echo "📊 Informations du binaire :"
	@file $(KERNEL_BIN) 2>/dev/null || true
	@echo ""
	@echo "📏 Taille du kernel :"
	@ls -lh $(KERNEL_BIN) 2>/dev/null || true

run: $(KERNEL_BIN)
	@echo "🚀 Lancement du kernel dans QEMU..."
	@echo "   (Appuyez sur Ctrl+C pour quitter)"
	@echo ""
	@qemu-system-i386 -kernel $(KERNEL_BIN)

run-iso: iso
	@echo "🚀 Lancement de l'ISO dans QEMU..."
	@echo "   (Appuyez sur Ctrl+C pour quitter)"
	@echo ""
	@qemu-system-i386 -cdrom $(ISO_FILE)

run-debug: $(KERNEL_BIN)
	@echo "🐛 Lancement en mode debug..."
	@qemu-system-i386 -kernel $(KERNEL_BIN) -d int,cpu_reset -no-reboot

# =============================================================================
# Nettoyage
# =============================================================================

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
	@echo "🧹 Nettoyage complet..."
	@rm -rf $(DIST)
	@echo "✅ Nettoyage complet terminé"

rebuild: clean all

# =============================================================================
# Informations et aide
# =============================================================================

info:
	@echo "╔════════════════════════════════════════════════════════════╗"
	@echo "║         myos-i686 - Informations du projet                ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ Compilateur    : $(CC)                               ║"
	@echo "║ Assembleur     : $(AS)                                    ║"
	@echo "║ Optimisation   : -O2                                       ║"
	@echo "║ Stack Protector: -fstack-protector-all                     ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ Kernel binaire : $(KERNEL_BIN)                    ║"
	@echo "║ Image ISO      : $(ISO_FILE)                          ║"
	@echo "╠════════════════════════════════════════════════════════════╣"
	@echo "║ Fichiers sources détectés :                                ║"
	@echo "╚════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "📄 Sources C :"
	@for file in $(C_SOURCES); do \
		echo "   • $$file"; \
	done
	@echo ""
	@echo "📄 Sources ASM :"
	@echo "   • $(BOOT_ASM)"
	@test -f $(ISR_ASM) && echo "   • $(ISR_ASM)" || true
	@echo ""
	@if [ -d $(KERNEL_BUILD) ] && [ "$$(ls -A $(KERNEL_BUILD) 2>/dev/null)" ]; then \
		echo "📦 Fichiers compilés :"; \
		ls -lh $(KERNEL_BUILD)/*.o 2>/dev/null | awk '{print "   " $$9 " (" $$5 ")"}' || true; \
		echo ""; \
		echo "📈 Taille totale :"; \
		du -sh $(KERNEL_BUILD) 2>/dev/null || true; \
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
	@echo "╚════════════════════════════════════════════════════════════╝"

list:
	@echo "📁 Fichiers du projet :"
	@echo ""
	@echo "📄 Obligatoires (Partie 1-2) :"
	@echo "   • boot.asm"
	@echo "   • kernel/src/kernel.c"
	@echo "   • kernel/src/printf.c"
	@echo "   • kernel/src/linker.ld"
	@echo ""
	@echo "📄 Optionnels (Parties 3-7) :"
	@test -f $(KERNEL_SRC)/stack_protector.c && echo "   ✓ stack_protector.c (Partie 3)" || echo "   ✗ stack_protector.c"
	@test -f $(KERNEL_SRC)/idt.c && echo "   ✓ idt.c (Partie 4)" || echo "   ✗ idt.c"
	@test -f $(KERNEL_SRC)/isr.c && echo "   ✓ isr.c (Partie 4)" || echo "   ✗ isr.c"
	@test -f $(KERNEL_SRC)/isr_asm.asm && echo "   ✓ isr_asm.asm (Partie 4)" || echo "   ✗ isr_asm.asm"
	@test -f $(KERNEL_SRC)/irq.c && echo "   ✓ irq.c (Partie 5)" || echo "   ✗ irq.c"
	@test -f $(KERNEL_SRC)/pic.c && echo "   ✓ pic.c (Partie 5)" || echo "   ✗ pic.c"
	@test -f $(KERNEL_SRC)/timer.c && echo "   ✓ timer.c (Partie 5)" || echo "   ✗ timer.c"
	@test -f $(KERNEL_SRC)/process.c && echo "   ✓ process.c (Partie 6)" || echo "   ✗ process.c"
	@test -f $(KERNEL_SRC)/scheduler.c && echo "   ✓ scheduler.c (Partie 6)" || echo "   ✗ scheduler.c"
	@test -f $(KERNEL_SRC)/keyboard.c && echo "   ✓ keyboard.c (Partie 7)" || echo "   ✗ keyboard.c"
	@test -f $(KERNEL_SRC)/shell.c && echo "   ✓ shell.c (Partie 7)" || echo "   ✗ shell.c"

.PHONY: all dirs clean clean-iso distclean rebuild check \
        info help iso run run-iso run-debug list
