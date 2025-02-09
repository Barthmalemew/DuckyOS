include config.mk

# Default target
.PHONY: all
all: kernel

# Include other makefiles
include src/boot/Makefile
include src/kernel/Makefile

# Clean build files
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*

# Run the OS in QEMU
.PHONY: run
run: all
	qemu-system-i386 -kernel $(BUILD_DIR)/kernel.bin

# Debug with GDB
.PHONY: debug
debug: all
	qemu-system-i386 -kernel $(BUILD_DIR)/kernel.bin -s -S &
	gdb -ex "target remote localhost:1234" \
	    -ex "symbol-file $(BUILD_DIR)/kernel.elf"
