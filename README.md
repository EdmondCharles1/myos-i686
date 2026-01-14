# myos-i686

Un système d'exploitation minimaliste pour architecture i686 (x86 32-bit).

## 🎯 Objectif

Apprendre le développement OS bare-metal en créant un kernel i686 bootable.

## 🛠️ Prérequis

- WSL2 (Ubuntu/Debian)
- Cross-compiler i686-elf-gcc
- NASM
- QEMU (pour les tests)

## 🚀 Quick Start
```bash
# Compiler
make

# Vérifier Multiboot
make check

# Tester dans QEMU
qemu-system-i386 -kernel kernel/build/myos.bin
```

## 📁 Structure
```
myos-i686/
├── boot.asm              # Point d'entrée assembleur
├── kernel/
│   ├── src/
│   │   ├── kernel.c      # Code C du kernel
│   │   └── linker.ld     # Script de linkage
│   └── build/            # Fichiers compilés (généré)
├── docs/
│   └── BUILD.md          # Documentation de build
├── scripts/              # Scripts d'automatisation
└── Makefile              # Système de build
```

## 📚 Documentation

Voir [docs/BUILD.md](docs/BUILD.md) pour les instructions détaillées.

## 🎨 Fonctionnalités actuelles

- [x] Boot via GRUB (Multiboot)
- [x] Affichage VGA mode texte 80x25
- [x] Gestion des couleurs
- [ ] Gestion clavier
- [ ] Interruptions (IDT)
- [ ] Pagination mémoire

## 📝 Licence

Projet éducatif - Libre d'utilisation
