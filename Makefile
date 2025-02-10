# Compiler settings
CROSS_COMPILE ?= i686-elf-
CC = $(HOME)/dev/tools/cross/bin/$(CROSS_COMPILE)gcc
ASM = nasm
LD = $(CROSS_COMPILE)ld

# Directories
KERNEL_DIR = src/kernel
DRIVER_DIR = src/drivers
BOOT_DIR = src/boot
SYSTEM_DIR = src/kernel

# Source files
BOOT_SRC = $(BOOT_DIR)/boot.asm
KERNEL_SRC = $(wildcard $(KERNEL_DIR)/*.c)
DRIVER_SRC = $(wildcard $(DRIVER_DIR)/*.c)
ASM_SRC = $(wildcard $(KERNEL_DIR)/*.asm)
SYSTEM_SRC = $(KERNEL_DIR)/string.c

# Object files
BOOT_OBJ = $(BOOT_SRC:.asm=.o)
KERNEL_OBJ = $(KERNEL_SRC:.c=.o)
DRIVER_OBJ = $(DRIVER_SRC:.c=.o)
ASM_OBJ = $(ASM_SRC:.asm=.o)

# All objects in correct order
OBJECTS = $(BOOT_OBJ) $(KERNEL_OBJ) $(ASM_OBJ) $(DRIVER_OBJ)

# Output files
Kernel = duckyos.bin
ISO = duckyos.iso

# Flags
CFLAGS = -ffreestanding -O2 -Wall -Wextra -I./src/include
LDFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

.PHONY: all clean run iso

all: $(Kernel)

$(Kernel): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c $(Headers)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm -f elf32 $< -o $@

clean:
	rm -f $(Kernel) $(ISO) $(OBJECTS)

run: $(Kernel)
	qemu-system-i386 -kernel $(Kernel)

iso: $(Kernel)
	mkdir -p isodir/boot/grub
	cp $(Kernel) isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o $(ISO) isodir
