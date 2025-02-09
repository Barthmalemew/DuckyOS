# Project configuration
ARCH := x86
BUILD_TYPE := debug

# Tool configuration
CC := gcc
AS := nasm
LD := ld

# Paths
BUILD_DIR := build
SRC_DIR := src

# Architecture-specific flags
ifeq ($(ARCH),x86)
    ASMFLAGS := -f elf32
    CFLAGS := -m32 -ffreestanding -O2 -Wall -Wextra
    LDFLAGS := -m elf_i386 -nostdlib
endif
