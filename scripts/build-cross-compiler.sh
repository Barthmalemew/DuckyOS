#!/bin/bash

# Exit on error
set -e

# Required packages for different distributions
DEBIAN_PACKAGES="build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo wget"
FEDORA_PACKAGES="gcc gcc-c++ make bison flex gmp-devel libmpc-devel mpfr-devel texinfo wget"
ARCH_PACKAGES="base-devel gmp libmpc mpfr texinfo wget"

# Detect package manager and install required packages
echo "Checking for required packages..."
if command -v apt-get >/dev/null 2>&1; then
    sudo apt-get update
    sudo apt-get install -y $DEBIAN_PACKAGES
elif command -v dnf >/dev/null 2>&1; then
    sudo dnf install -y $FEDORA_PACKAGES
elif command -v pacman >/dev/null 2>&1; then
    sudo pacman -Sy --needed $ARCH_PACKAGES
else
    echo "Unsupported package manager. Please install build dependencies manually."
    exit 1
fi

# Check if running with proper permissions
if [ ! -w "$HOME/dev/tools" ]; then
    mkdir -p "$HOME/dev/tools"
    if [ $? -ne 0 ]; then
        echo "Error: Cannot create directory $HOME/dev/tools. Please check permissions."
        exit 1
    fi
fi

# Configuration - Use dev directory instead of opt
export PREFIX="$HOME/dev/tools/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Download URLs
BINUTILS_VERSION="2.40"
GCC_VERSION="13.2.0"
BINUTILS_URL="https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz"
GCC_URL="https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz"

# Create directories
mkdir -p "$HOME/dev/tools/cross"
mkdir -p "$HOME/dev/tools/src"
cd "$HOME/dev/tools/src"

# Check for required tools
for tool in wget make gcc g++ bison flex; do
    if ! command -v $tool >/dev/null 2>&1; then
        echo "Error: Required tool '$tool' is not installed."
        exit 1
    fi
done

# Download and extract sources
wget -nc "$BINUTILS_URL"
wget -nc "$GCC_URL"
tar xf "binutils-$BINUTILS_VERSION.tar.gz"
tar xf "gcc-$GCC_VERSION.tar.gz"

# Build binutils
mkdir -p build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror --disable-documentation
make
make install

# Build GCC
cd "$HOME/dev/tools/src"
mkdir -p build-gcc
cd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

echo "Cross-compiler installation complete!"
echo "Add this to your ~/.bashrc:"
echo "export PATH=\"$PREFIX/bin:\$PATH\""
