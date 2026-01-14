# myos-i686

Un système d'exploitation minimal en bare-metal pour architecture x86 (32-bit).

## 🎯 Caractéristiques

- **Architecture** : i686 (Intel x86 32-bit)
- **Bootloader** : Compatible Multiboot (GRUB)
- **Langage** : C11 + Assembleur (NASM)
- **Compilateur** : Cross-compiler i686-elf-gcc
- **Fonctionnalités** :
  - Affichage VGA mode texte 80x25
  - Fonction `printf()` complète avec formatage
  - Support des formats : `%c %s %d %u %x %X %p %%`

## 📁 Structure du projet
```
myos-i686/
├── boot.asm              # Point d'entrée assembleur (Multiboot)
├── kernel/
│   ├── src/
│   │   ├── kernel.c      # Code principal du kernel
│   │   ├── printf.c      # Implémentation de printf
│   │   ├── printf.h      # Header printf
│   │   └── linker.ld     # Script de linkage
│   └── build/            # Fichiers compilés (ignoré par Git)
├── scripts/
│   ├── run-qemu.sh       # Script pour lancer QEMU
│   └── toolchain/
│       └── build-toolchain.sh  # Script de build du cross-compiler
├── docs/
│   └── BUILD.md          # Documentation de build
├── Makefile              # Système de build
└── README.md             # Ce fichier
```

## 🛠️ Prérequis

### 1. Cross-compiler i686-elf-gcc

Le projet nécessite un cross-compiler i686-elf installé dans `$HOME/opt/cross`.

**Installation** : voir le script `scripts/toolchain/build-toolchain.sh`

### 2. Dépendances système
```bash
sudo apt update
sudo apt install -y nasm make grub-pc-bin xorriso qemu-system-x86
```

## 🔨 Compilation
```bash
# Compiler le kernel
make

# Nettoyer les fichiers de build
make clean

# Recompiler complètement
make rebuild

# Vérifier le header Multiboot
make check

# Afficher les informations du projet
make info
```

## 🚀 Exécution

### Test direct avec QEMU (sans ISO)
```bash
qemu-system-i386 -kernel kernel/build/myos.bin
```

### Avec le script fourni
```bash
./scripts/run-qemu.sh
```

## 📚 Documentation

- **Guide de build** : `docs/BUILD.md`
- **OSDev Wiki** : https://wiki.osdev.org/
- **Formatted Printing** : https://wiki.osdev.org/Formatted_Printing

## 🧪 Tests

Le kernel affiche :
- Un en-tête avec le nom du système
- Tests de formatage printf (caractères, nombres, hexadécimal, pointeurs)
- Informations système

## 🎓 Apprentissage

Ce projet est créé dans un but éducatif pour comprendre :
- Le fonctionnement d'un bootloader
- La programmation bare-metal
- L'accès direct au hardware (VGA)
- La création d'un cross-compiler
- Le développement OS from scratch

## 📝 TODO

- [ ] Création d'une image ISO bootable
- [ ] Gestion des interruptions (IDT)
- [ ] Support clavier
- [ ] Gestionnaire de mémoire
- [ ] Pagination
- [ ] Multitâche

## 📜 Licence

Ce projet est sous licence MIT - voir le fichier LICENSE pour plus de détails.

## 🙏 Ressources

- [OSDev Wiki](https://wiki.osdev.org/)
- [Bare Bones Tutorial](https://wiki.osdev.org/Bare_Bones)
- [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler)

## 👤 Auteur

Projet personnel d'apprentissage du développement OS.
