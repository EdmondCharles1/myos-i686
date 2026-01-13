# Build Guide

## Prerequisites
```bash
sudo apt install -y nasm grub-pc-bin xorriso qemu-system-x86
```

## Build Steps

### 1. Build toolchain (one-time setup)
```bash
cd toolchain
./build-toolchain.sh
```

### 2. Build kernel
```bash
cd kernel
make
```

### 3. Run in QEMU
```bash
make run
# or
cd ..
./scripts/run-qemu.sh
```

## Expected Output

You should see:
```
MyOS i686 v0.1
Kernel loaded successfully!
```

## Troubleshooting

### `i686-elf-gcc: command not found`
```bash
export PATH="$HOME/opt/cross/bin:$PATH"
```

### QEMU shows black screen
```bash
grub-file --is-x86-multiboot kernel/build/myos.bin
```
Should return exit code 0.
