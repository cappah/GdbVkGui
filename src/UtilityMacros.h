#pragma once

#include <inttypes.h>
#include <string.h>

#define UNUSED_VAR(x) (void)x
#define ENUM_VAL(x) 1 << x
#define BIT(x) (1 << (x))

#define ROUNDUP(x, granularity) (((x) + (granularity)-1) & ~((granularity)-1))
#define MAX(x, y) ((x > y) ? (x) : (y))
#define MIN(x, y) ((x < y) ? (x) : (y))
#define CLAMP(x, min, max)                                                     \
    (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define COMPARE(x, y) ((x) < (y) ? -1 : ((x) == (y) ? 0 : 1))

#define STRINGIFY_EXPANDED(a) #a
#define STRINGIFY(a) STRINGIFY_EXPANDED(a)
#define STR_EQ(a, b) (memcmp((a), (b), strlen(a)) == 0)

#define STATIC_ARRAY_COUNT(X) (sizeof(X) / sizeof(X[0]))
#define MEM_OFFSET(PTR, COUNT) ((void*)((unsigned char*)(PTR) + (COUNT)))
#define MEM_ZERO(PTR) memset(PTR, 0, sizeof(PTR[0]))
#define MEM_ZERO_ARRAY(PTR) memset(PTR, 0, sizeof(PTR))
#define BYTE_COUNT(PTR1, PTR2) (uint32_t)((uintptr_t)(PTR1) - (uintptr_t)(PTR2))
