OBJECTS = kernel_main.o boot.oo
CC = ../bin/i686-elf-gcc
CFLAGS = -I../include -I./include -Wall -Wextra -O2 -ffreestanding -std=gnu99 -c
LD = ../bin/i686-elf-gcc
LDFLAGS = -lgcc -T ./linker.ld -ffreestanding -O2 -nostdlib
AS = nasm
ASFLAGS = -felf32

.phony: clean

all: duckyos.bin

duckyos.bin: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.oo: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -vrf *.o duckyos.bin *.oo

