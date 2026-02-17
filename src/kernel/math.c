#include "../include/math.h"
#include <stdint.h>

// Simple fixed-point matrix multiplication (row-major)
// C = A * B
void mat_mul_fixed(const int32_t *A, const int32_t *B, int32_t *C, uint32_t n, uint8_t frac_bits) {
    for (uint32_t i = 0; i < n; ++i) {
        for (uint32_t j = 0; j < n; ++j) {
            int64_t acc = 0;
            for (uint32_t k = 0; k < n; ++k) {
                int64_t a = A[i * n + k];
                int64_t b = B[k * n + j];
                acc += a * b; // product has 2*frac_bits fractional bits
            }
            // Downshift back to frac_bits with rounding
            int64_t rounding = (int64_t)1 << (frac_bits - 1);
            acc = (acc + rounding) >> frac_bits;
            C[i * n + j] = (int32_t)acc;
        }
    }
}
