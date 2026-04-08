#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdint.h>
#include "screen.h"

// Define standard types for lwIP
typedef uint8_t    u8_t;
typedef int8_t     s8_t;
typedef uint16_t   u16_t;
typedef int16_t    s16_t;
typedef uint32_t   u32_t;
typedef int32_t    s32_t;

typedef uintptr_t  mem_ptr_t;

// Endianness
#define BYTE_ORDER LITTLE_ENDIAN
#define LWIP_NO_CTYPE_H 1

// Diagnostic/Debugging
#define LWIP_PLATFORM_DIAG(x)   // { print_string("LWIP: "); } // We can improve printf later
#define LWIP_PLATFORM_ASSERT(x) { print_string("LWIP ASSERT: "); print_string(x); print_string("\n"); while(1); }

// Type alignment and structure packing
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x

#endif
