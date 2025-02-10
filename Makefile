# Compiler settings
CROSS_COMPILE ?= i686-elf-
CC = $(HOME)/dev/tools/cross/bin/$(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

Assembler = nasm
Linker = $(LD)

LinkerFlags += -nostartfiles

CompilerFlags = -ffreestanding -O2 -Wall -Wextra -I./src/include

# Source files
C_Sources = $(wildcard src/kernel/*.c) $(wildcard src/drivers/*.c)
Assembly_Sources = $(wildcard src/kernel/*.asm)
Headers = $(wildcard src/include/*.h)

# Object files
ObjectFiles = ${C_Sources:.c=.o} ${Assembly_Sources:.asm=.o}

# Output files
Kernel = duckyos.bin
ISO = duckyos.iso

.PHONY: all clean run iso

all: $(Kernel)

$(Kernel): $(ObjectFiles)
	$(CC) -T linker.ld -o $@ $(LinkerFlags) $(ObjectFiles)

%.o: %.c $(Headers)
	$(CC) $(CompilerFlags) -c $< -o $@

%.o: %.asm
	nasm -f elf32 $< -o $@

clean:
	rm -f $(Kernel) $(ISO) $(ObjectFiles)

run: $(Kernel)
	qemu-system-i386 -kernel $(Kernel)

iso: $(Kernel)
	mkdir -p isodir/boot/grub
	cp $(Kernel) isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $(ISO) isodir
