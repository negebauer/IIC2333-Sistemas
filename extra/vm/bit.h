#pragma once
#include "limits.h"

#define bit_i(bitNum)        (1 << bitNum)
#define bit_set(x, bitNum)   (x |=  bit_i(bitNum))
#define bit_clear(x, bitNum) (x &= ~bit_i(bitNum))

#define BITS (CHAR_BIT*sizeof(int))
#define mask_left(l)  (~0 << (BITS-l))
#define mask_right(r) (~(~0 << r))
#define mask_range(w) (~(~0 << m) << n)

