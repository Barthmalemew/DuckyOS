#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default settings
MEMORY=128M
ENABLE_DEBUG=0
ENABLE_SERIAL=0

# Function to display help
show_help() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -d, --debug     Enable GDB debugging server"
    echo "  -s, --serial    Enable serial output"
    echo "  -m, --memory    Set memory size (default: 128M)"
    echo "  -h, --help      Show this help message"
}

# Check dependencies
check_dependencies() {
    local missing=0
    
    if ! command -v qemu-system-i386 >/dev/null 2>&1; then
        echo -e "${RED}Error: qemu-system-i386 is not installed${NC}"
        missing=1
    fi
    
    if ! command -v make >/dev/null 2>&1; then
        echo -e "${RED}Error: make is not installed${NC}"
        missing=1
    fi
    
    if [ $missing -eq 1 ]; then
        exit 1
    fi
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            ENABLE_DEBUG=1
            shift
            ;;
        -s|--serial)
            ENABLE_SERIAL=1
            shift
            ;;
        -m|--memory)
            MEMORY="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Check dependencies
check_dependencies

# Build the operating system
echo -e "${YELLOW}Building DuckyOS...${NC}"
make clean && make
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Prepare QEMU arguments
QEMU_ARGS="-kernel duckyos.bin -m $MEMORY"

# Add debug support if enabled
if [ $ENABLE_DEBUG -eq 1 ]; then
    QEMU_ARGS="$QEMU_ARGS -s -S"
    echo -e "${GREEN}Debug mode enabled. Connect with: gdb -ex 'target remote localhost:1234'${NC}"
fi

# Add serial output if enabled
if [ $ENABLE_SERIAL -eq 1 ]; then
    QEMU_ARGS="$QEMU_ARGS -serial stdio"
    echo -e "${GREEN}Serial output enabled${NC}"
fi

# Run QEMU
echo -e "${GREEN}Starting DuckyOS...${NC}"
qemu-system-i386 $QEMU_ARGS
