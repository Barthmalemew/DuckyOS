#ifndef TYPE_H
#define TYPE_H

#include <stddef.h>
#include <stdint.h>

typedef enum { false = 0, true = 1 } boolean;

// Common type definitions
typedef unsigned char unsigned_eight_bit_integer;
typedef unsigned short unsigned_sixteen_bit_integer;
typedef unsigned int unsigned_thirty_two_bit_integer;
typedef unsigned long long unsigned_sixty_four_bit_integer;

typedef signed char signed_eight_bit_integer;
typedef signed short signed_sixteen_bit_integer;
typedef signed int signed_thirty_two_bit_integer;
typedef signed long long signed_sixty_four_bit_integer;

#endif
