/**
 * @file kernel.c
 * @brief Kernel entry point and main functionality for DuckOS
 * 
 * This file contains the core kernel implementation for DuckOS, a basic operating
 * system. The kernel is responsible for initializing hardware components and
 * maintaining system state. Currently, it performs basic VGA initialization
 * and enters a halt state.
 *
 * System Requirements:
 * - x86 compatible processor
 * - VGA compatible display hardware
 * - Properly configured bootloader
 */

/* 
 * External VGA Interface Functions
 * These functions are implemented elsewhere in the VGA driver code.
 * The kernel relies on them for display output functionality.
 */

/**
 * @brief Initialize the VGA display hardware
 * 
 * This function must be called before any other VGA functions.
 * It sets up the hardware interface and prepares the display
 * for text output. The specific initialization steps are
 * implementation-dependent.
 */
extern void init_vga(void);

/**
 * @brief Write text to the VGA display
 * 
 * @param data Null-terminated string to be displayed
 * 
 * Outputs text to the VGA display at the current cursor position.
 * Handles special characters like newlines ('\n') appropriately.
 * The text is displayed using the current color settings.
 */
extern void vga_write(const char* data);

/**
 * @brief Set the text color for VGA output
 * 
 * @param fg Foreground color value (text color)
 * @param bg Background color value
 * 
 * Configures the colors used for subsequent text output.
 * Color values are implementation-specific and should be
 * defined in the VGA driver documentation.
 */
extern void vga_setcolor(int fg, int bg);

/**
 * @brief Main kernel initialization and execution function
 * 
 * This function is responsible for:
 * 1. Initializing core hardware components (currently just VGA)
 * 2. Displaying initial boot messages
 * 3. Maintaining system state via halt loop
 * 
 * Future enhancements could include:
 * - Memory management initialization
 * - Interrupt handler setup
 * - Process scheduler startup
 * - Device driver initialization
 * - System calls setup
 */
void kernel_main(void) {
    /* 
     * Initialize VGA hardware through its implementation
     * This must be done before any display output attempts 
     */
    init_vga();
    
    /* 
     * Display boot message using VGA implementation
     * This confirms that initialization was successful
     */
    vga_write("DuckOS Booting...\n");
    
    /* 
     * Enter infinite halt loop to maintain system state
     * The HLT instruction:
     * - Stops CPU execution until an interrupt occurs
     * - Reduces power consumption
     * - Prevents execution of invalid memory
     * 
     * Future enhancement: This could be replaced with:
     * - Process scheduler
     * - Power management system
     * - Idle task manager
     */
    while(1) {
        /* 
         * Execute CPU halt instruction
         * volatile keyword prevents optimization removing the loop
         */
        __asm__ volatile("hlt");
    }
}

/**
 * @brief Entry point from assembly bootloader
 * 
 * This function is called directly from boot.asm after:
 * - CPU initialization
 * - Stack setup
 * - Basic hardware configuration
 * 
 * It provides a clean separation between assembly and C code,
 * allowing for proper C environment setup before kernel execution.
 */
void main(void) {
    /* 
     * Transfer control to main kernel function
     * This additional layer of abstraction allows for:
     * - Future initialization code if needed
     * - Clean separation of concerns
     * - Easier debugging
     */
    kernel_main();
}
