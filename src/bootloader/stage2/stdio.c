/******************************************************************************
 *  FILE: printf_heavily_documented.c
 *  DESCRIPTION:
 *      A heavily documented single-file example of a simplified printf-like
 *      implementation for x86 environments. It uses a custom low-level
 *      function to write characters directly to the screen (x86_Video_WriteCharTeletype).
 *
 *      This file provides:
 *          - putc(...)   : Output a single character
 *          - puts(...)   : Output a standard (near) string
 *          - puts_f(...) : Output a far string
 *          - printf(...) : A simplified printf implementation
 *          - printf_number(...) : Helper function to handle integer conversions
 *
 *      NOTE:
 *      - The code uses some assumptions about x86 calling conventions for
 *        handling variable arguments.
 *      - The code expects definitions for:
 *           x86_Video_WriteCharTeletype(...)
 *           x86_div64_32(...)
 *        in "x86.h", which is not provided here.
 *      - The code is for demonstration/learning in a low-level or kernel-like
 *        environment and is not meant to be a fully robust replacement for
 *        the C standard library's printf.
 ******************************************************************************/

#include "stdio.h"   /* Potentially for function declarations. */
#include "x86.h"     /* Must contain declarations for x86-specific routines. */

/******************************************************************************
 * putc
 * ----------------------------------------------------------------------------
 * Writes a single character to the screen using x86_Video_WriteCharTeletype().
 * This function is the simplest possible wrapper around the low-level API.
 ******************************************************************************/
void putc(char c)
{
    /* x86_Video_WriteCharTeletype(char c, int attribute)
     * In typical VGA text mode, 'attribute' might control color or other text
     * attributes. Here we pass 0 for default or inherited attributes. */
    x86_Video_WriteCharTeletype(c, 0);
}

/******************************************************************************
 * puts
 * ----------------------------------------------------------------------------
 * Outputs a null-terminated string (in "near" memory) by repeatedly calling putc.
 * Continues until the end of the string (the '\0' terminator) is encountered.
 ******************************************************************************/
void puts(const char* str)
{
    /* Loop until we hit a null character. */
    while(*str)
    {
        putc(*str); /* Output the current character. */
        str++;      /* Advance the pointer to the next character. */
    }
}

/******************************************************************************
 * puts_f
 * ----------------------------------------------------------------------------
 * Outputs a null-terminated string in "far" memory. This is relevant in certain
 * memory models (e.g., 16-bit x86 with separate segment:offset pointers).
 * The logic is identical to puts(), except it expects a far pointer.
 ******************************************************************************/
void puts_f(const char far* str)
{
    /* Loop until we hit a null character. */
    while(*str)
    {
        putc(*str); /* Output the current character. */
        str++;      /* Advance the pointer to the next character in far memory. */
    }
}

/******************************************************************************
 * The following #defines implement a state machine for our simplified printf.
 * 
 * PRINTF_STATE_* constants represent parser states:
 *   - NORMAL:        Scanning non-format characters or waiting for a '%'.
 *   - LENGTH:        We have just read '%'; next characters might be length
 *                    modifiers (e.g., 'h', 'l').
 *   - LENGTH_SHORT:  We saw 'h' once; might see another 'h' for "hh".
 *   - LENGTH_LONG:   We saw 'l' once; might see another 'l' for "ll".
 *   - SPEC:          Ready to parse the actual format specifier (e.g., 'd', 'x', etc.)
 *
 * PRINTF_LENGTH_* constants represent the size/length modifiers encountered:
 *   - DEFAULT       : No length modifier, treat as regular int or unsigned int.
 *   - SHORT_SHORT   : "hh" modifier, typically char or unsigned char.
 *   - SHORT         : "h" modifier, typically short or unsigned short.
 *   - LONG          : "l" modifier, typically long or unsigned long.
 *   - LONG_LONG     : "ll" modifier, typically long long or unsigned long long.
 ******************************************************************************/
#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4

/******************************************************************************
 * Forward declaration for the helper function that prints numbers:
 ******************************************************************************/
int* printf_number(int* argp, int length, bool sign, int radix);

/******************************************************************************
 * printf
 * ----------------------------------------------------------------------------
 * A simplified printf-like function with limited support for:
 *   - %c  : character
 *   - %s  : string
 *   - %d, %i : signed integer (decimal)
 *   - %u  : unsigned integer (decimal)
 *   - %o  : unsigned integer (octal)
 *   - %x, %X, %p : unsigned integer (hexadecimal)
 *   - %%  : literal '%'
 * 
 * Supports length modifiers:
 *   - h   : short
 *   - hh  : char
 *   - l   : long
 *   - ll  : long long
 * 
 * The code uses manual argument pointer (argp) manipulation to access the
 * variadic arguments. This is non-portable but acceptable in certain low-level
 * contexts, especially older x86 environments.
 ******************************************************************************/
void _cdecl printf(const char* fmt, ...)
{
    /* 
     * We cast &fmt (address of the first parameter) to int*, then increment it
     * to skip over 'fmt' itself. That gets us to the first variadic argument.
     */
    int* argp = (int*)&fmt;
    
    /* State machine tracking:
     *   - state: current parser state (normal vs. length vs. spec).
     *   - length: length modifier (default, short, long, etc.).
     *   - radix: default base 10. 
     *   - sign: whether the printed number should be signed.
     */
    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;

    /* Move 'argp' to point to the first variadic argument. */
    argp++;

    /* Iterate over every character in the format string until we reach '\0'. */
    while (*fmt)
    {
        switch (state)
        {
            /******************************************************************
             * 1) PRINTF_STATE_NORMAL
             *    We are scanning regular text for a '%' to enter formatting.
             ******************************************************************/
            case PRINTF_STATE_NORMAL:
                switch (*fmt)
                {
                    /* If we see '%', move to PRINTF_STATE_LENGTH to check
                     * for optional length modifiers. */
                    case '%':   
                        state = PRINTF_STATE_LENGTH;
                        break;

                    /* Otherwise, it's just a normal character. Print it. */
                    default:    
                        putc(*fmt);
                        break;
                }
                break;

            /******************************************************************
             * 2) PRINTF_STATE_LENGTH
             *    Just saw '%'. Check if the next character is 'h' or 'l'
             *    for length modifiers. If not, go directly to spec.
             ******************************************************************/
            case PRINTF_STATE_LENGTH:
                switch (*fmt)
                {
                    /* 'h' -> short; next might be another 'h'. */
                    case 'h':   
                        length = PRINTF_LENGTH_SHORT;
                        state = PRINTF_STATE_LENGTH_SHORT;
                        break;

                    /* 'l' -> long; next might be another 'l'. */
                    case 'l':   
                        length = PRINTF_LENGTH_LONG;
                        state = PRINTF_STATE_LENGTH_LONG;
                        break;

                    /* Otherwise, go straight to final spec parsing. */
                    default:    
                        goto PRINTF_STATE_SPEC_;
                }
                break;

            /******************************************************************
             * 3) PRINTF_STATE_LENGTH_SHORT
             *    We encountered one 'h'. If we see another 'h', it means 'hh'.
             ******************************************************************/
            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h')
                {
                    length = PRINTF_LENGTH_SHORT_SHORT; /* "hh" */
                    state = PRINTF_STATE_SPEC;
                }
                else 
                {
                    /* If no second 'h', handle spec as is. */
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            /******************************************************************
             * 4) PRINTF_STATE_LENGTH_LONG
             *    We encountered one 'l'. If we see another 'l', it means 'll'.
             ******************************************************************/
            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l')
                {
                    length = PRINTF_LENGTH_LONG_LONG; /* "ll" */
                    state = PRINTF_STATE_SPEC;
                }
                else 
                {
                    /* If no second 'l', handle spec as is. */
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            /******************************************************************
             * 5) PRINTF_STATE_SPEC (and PRINTF_STATE_SPEC_ label)
             *    We now parse the actual specifier: 'c', 's', 'd', 'u', etc.
             ******************************************************************/
            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt)
                {
                    /* %c: Print a single character. */
                    case 'c':   
                        putc((char)*argp);
                        argp++;
                        break;

                    /* %s: Print a string.
                     * If length modifier is 'l' or 'll', assume far pointer. 
                     * Otherwise, assume normal pointer. */
                    case 's':   
                        if (length == PRINTF_LENGTH_LONG || length == PRINTF_LENGTH_LONG_LONG) 
                        {
                            /* For 'far' strings, we skip 2 stack units. 
                             * This is environment-specific. */
                            puts_f(*(const char far**)argp);
                            argp += 2;
                        }
                        else 
                        {
                            /* For normal strings, skip 1 stack unit. */
                            puts(*(const char**)argp);
                            argp++;
                        }
                        break;

                    /* %%: Print literal '%'. */
                    case '%':   
                        putc('%');
                        break;

                    /* %d or %i: Signed integer, decimal. */
                    case 'd':
                    case 'i':   
                        radix = 10; 
                        sign = true;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    /* %u: Unsigned decimal. */
                    case 'u':   
                        radix = 10; 
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    /* %x, %X, or %p: Unsigned hex. 
                     * For simplicity, 'X' is treated like 'x' in this code. */
                    case 'X':
                    case 'x':
                    case 'p':   
                        radix = 16; 
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    /* %o: Unsigned octal. */
                    case 'o':   
                        radix = 8;  
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;

                    /* Any unknown specifier is ignored. */
                    default:    
                        break;
                }

                /* Once we've handled the specifier, reset everything 
                 * back to default for the next round. */
                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = false;
                break;
        }

        /* Move to the next character in the format string. */
        fmt++;
    }
}

/******************************************************************************
 * g_HexChars
 * ----------------------------------------------------------------------------
 * Used by printf_number() for converting numeric values to their hexadecimal
 * representation. The array includes digits 0-9 and letters a-f.
 ******************************************************************************/
const char g_HexChars[] = "0123456789abcdef";

/******************************************************************************
 * printf_number
 * ----------------------------------------------------------------------------
 * This helper function handles extracting the appropriate numeric argument
 * from the stack (based on 'length'), determines if the value is negative
 * (for signed formats), and then converts the number into a string in the
 * specified 'radix' (base 10, 16, or 8). It then prints that string.
 *
 * Parameters:
 *   - argp   : Pointer to the current argument in the variadic list
 *   - length : Length modifier (default, short, long, etc.)
 *   - sign   : Indicates signed (true) or unsigned (false)
 *   - radix  : The numeric base in which to print (10, 16, 8, etc.)
 *
 * Returns:
 *   - Updated argp (moved past the consumed argument)
 ******************************************************************************/
int* printf_number(int* argp, int length, bool sign, int radix)
{
    /* Temporary buffer for the string representation (in reverse). */
    char buffer[32];

    /* We'll store the final numeric value in 'number' as unsigned long long. */
    unsigned long long number;

    /* Tracks if the original number was negative: 
     *   1  -> positive
     *  -1  -> negative
     */
    int number_sign = 1;

    /* Position index for building 'buffer' in reverse. */
    int pos = 0;

    /**************************************************************************
     * 1) Read the correct number of bytes/words from the stack based on
     *    the length specifier. Also handle sign extension if needed.
     **************************************************************************/
    switch (length)
    {
        /* No modifier, 'h', or 'hh' all end up reading one 'int' unit
         * on the stack in many ABIs. This is environment-specific but
         * works as a simplified approach here. */
        case PRINTF_LENGTH_SHORT_SHORT:
        case PRINTF_LENGTH_SHORT:
        case PRINTF_LENGTH_DEFAULT:
            if (sign)
            {
                int n = *argp;  /* signed int */
                if (n < 0)
                {
                    n = -n;          /* make it positive */
                    number_sign = -1;/* record that it was negative */
                }
                number = (unsigned long long)n;
            }
            else
            {
                /* read an unsigned int */
                number = *(unsigned int*)argp;
            }
            argp++;
            break;

        /* 'l' -> might need 2 stack units for a 32-bit long 
         * depending on compiler and environment. */
        case PRINTF_LENGTH_LONG:
            if (sign)
            {
                long int n = *(long int*)argp;
                if (n < 0)
                {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            }
            else
            {
                number = *(unsigned long int*)argp;
            }
            /* Move argp by 2 ints on the stack (ABI-specific). */
            argp += 2;
            break;

        /* 'll' -> might need 4 stack units for a 64-bit long long. */
        case PRINTF_LENGTH_LONG_LONG:
            if (sign)
            {
                long long int n = *(long long int*)argp;
                if (n < 0)
                {
                    n = -n;
                    number_sign = -1;
                }
                number = (unsigned long long)n;
            }
            else
            {
                number = *(unsigned long long int*)argp;
            }
            /* Move argp by 4 ints on the stack (ABI-specific). */
            argp += 4;
            break;
    }

    /**************************************************************************
     * 2) Convert the numeric value to the desired base (radix).
     *    We'll do this by repeated division, storing the remainder in
     *    buffer[pos++] in reverse order (LS digit first).
     **************************************************************************/
    do 
    {
        uint32_t rem;

        /* x86_div64_32(number, divisor, &result, &remainder)
         * This is presumably a 64-bit / 32-bit division provided by x86.h 
         * that updates 'number' with the quotient and 'rem' with the remainder. */
        x86_div64_32(number, radix, &number, &rem);

        /* Convert remainder (0 .. base-1) to a character. If base=16,
         * 0->'0', 1->'1', ... 10->'a', etc. */
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    /**************************************************************************
     * 3) If this was a signed format and the original number was negative,
     *    add a '-' sign to the buffer.
     **************************************************************************/
    if (sign && number_sign < 0)
    {
        buffer[pos++] = '-';
    }

    /**************************************************************************
     * 4) Print the contents of buffer[] in reverse order (because the digits
     *    were stored from least significant to most significant).
     **************************************************************************/
    while (--pos >= 0)
    {
        putc(buffer[pos]);
    }

    /* Return the updated argument pointer. */
    return argp;
}
