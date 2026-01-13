# MyOS i686 - Operating System from Scratch

Custom 32-bit operating system written in C and Assembly for learning purposes.

## 🎯 Project Status

- [x] Cross-compiler toolchain (i686-elf)
- [x] Multiboot-compliant bootloader
- [x] Basic VGA text mode driver
- [ ] Keyboard driver
- [ ] Interrupt handling (IDT)
- [ ] Memory management (paging)
- [ ] Multitasking

## 🛠️ Build Requirements

- **Host OS:** WSL2 (Ubuntu/Debian)
- **Cross-compiler:** i686-elf-gcc
- **Emulator:** QEMU (qemu-system-i386)
- **Bootloader:** GRUB

## 📦 Installation

### 1. Build the toolchain
```bash
cd toolchain
./build-toolchain.sh
```

### 2. Build the kernel
```bash
cd kernel
make
```

### 3. Run in QEMU
```bash
make run
# or
qemu-system-i386 -cdrom myos.iso
```

## 📚 Documentation

See [docs/BUILD.md](docs/BUILD.md) for detailed build instructions.

## 🔗 Resources

- [OSDev Wiki](https://wiki.osdev.org/)
- [GCC Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler)
- [Bare Bones Tutorial](https://wiki.osdev.org/Bare_Bones)

## 📝 License

MIT License - Feel free to learn from this code!
