# myos-i686 - Guide de Build

## Prérequis

- Cross-compiler i686-elf-gcc installé dans `$HOME/opt/cross`
- NASM (assembleur)
- Make
- GRUB tools (pour créer l'ISO)
- QEMU (pour tester)

## Installation des dépendances
```bash
sudo apt install -y nasm make grub-pc-bin xorriso qemu-system-x86
```

## Compilation
```bash
# Compiler le kernel
make

# Vérifier le Multiboot header
make check

# Nettoyer
make clean

# Recompiler complètement
make rebuild
```

## Structure des fichiers générés
```
kernel/build/
├── boot.o          # Code assembleur compilé
├── kernel.o        # Code C compilé
└── myos.bin        # Kernel final (ELF 32-bit)
```

## Commandes utiles
```bash
# Afficher les informations du projet
make info

# Afficher la structure
make tree

# Vérifier le type de fichier
file kernel/build/myos.bin

# Désassembler
i686-elf-objdump -d kernel/build/myos.bin | less

# Voir les symboles
i686-elf-nm kernel/build/myos.bin
```

## Dépannage

### Erreur : `i686-elf-gcc: command not found`

Vérifier le PATH :
```bash
export PATH="$HOME/opt/cross/bin:$PATH"
source ~/.bashrc
```

### Erreur : `nasm: command not found`

Installer NASM :
```bash
sudo apt install -y nasm
```

### Erreur : `undefined reference to '__udivdi3'`

Vérifier libgcc :
```bash
i686-elf-gcc -print-libgcc-file-name
```

## Stack Smashing Protector (SSP)

### Qu'est-ce que c'est ?

Le SSP est une protection contre les buffer overflows qui :
- Insère un "canary" (valeur de garde) sur la pile
- Vérifie le canary avant chaque retour de fonction
- Appelle `__stack_chk_fail()` si corruption détectée

### Activation

Le SSP est activé automatiquement via le flag GCC :
```bash
-fstack-protector-all
```

### Tester le SSP

⚠️ **Pour développeurs seulement - ceci crashera le kernel**

1. Décommenter `test_stack_smashing()` dans `kernel.c`
2. Recompiler : `make rebuild`
3. Lancer : `make run`
4. Observer la détection du buffer overflow

### Désactiver le SSP (non recommandé)

Si nécessaire, retirer le flag `-fstack-protector-all` du Makefile.

### Amélioration future

Pour un kernel en production, le canary devrait être :
- Généré aléatoirement au boot
- Basé sur une source d'entropie (RDRAND, timer, etc.)
- Différent à chaque exécution
