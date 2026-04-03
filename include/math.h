#ifndef MATH_H
#define MATH_H

#include <stdint.h>

// Fixed-point matrix multiply (square matrices)
// All values are 32-bit signed fixed-point with `FRAC_BITS` fractional bits.
// A, B, and out C are arrays of size n*n in row-major order.
void mat_mul_fixed(const int32_t *A, const int32_t *B, int32_t *C, uint32_t n, uint8_t frac_bits);

float sqrtf(float x);
float expf(float x);
float tanhf(float x);

#endif
