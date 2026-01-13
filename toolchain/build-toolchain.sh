#!/bin/bash
set -e

echo "=== Building i686-elf cross-compiler ==="

# Configuration
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

BINUTILS_VERSION=2.41
GCC_VERSION=13.2.0

# Installation des dépendances
echo "Installing dependencies..."
sudo apt update
sudo apt install -y build-essential bison flex libgmp3-dev libmpc-dev \
    libmpfr-dev texinfo wget

# Création des répertoires
mkdir -p $PREFIX
mkdir -p ~/src
cd ~/src

# Téléchargement
if [ ! -f "binutils-$BINUTILS_VERSION.tar.xz" ]; then
    wget https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.xz
fi
if [ ! -f "gcc-$GCC_VERSION.tar.xz" ]; then
    wget https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz
fi

# Extraction
tar -xf binutils-$BINUTILS_VERSION.tar.xz
tar -xf gcc-$GCC_VERSION.tar.xz

# Build Binutils
echo "Building Binutils..."
mkdir -p build-binutils && cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" \
    --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install

# Build GCC
echo "Building GCC..."
cd ~/src
mkdir -p build-gcc && cd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c --without-headers
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make install-gcc
make install-target-libgcc

# Vérification
echo ""
echo "=== Toolchain installed successfully ==="
$PREFIX/bin/i686-elf-gcc --version
echo ""
echo "Add to your ~/.bashrc:"
echo "export PATH=\"\$HOME/opt/cross/bin:\$PATH\""
