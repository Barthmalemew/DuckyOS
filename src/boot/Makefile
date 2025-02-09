# Boot directory Makefile

# Default target
all: boot.bin

# Compile boot.asm to a flat binary
boot.bin: boot.asm
	nasm -f bin -o ../../build/boot.bin boot.asm

# Clean build files
clean:
	rm -f ../../build/boot.bin

.PHONY: all clear
