// libwasm.h

#ifndef LIBWASM_H

#include <stdint.h>
#include <math.h>

#ifndef NOHARDWARE_SUPPORT
#  ifdef __GNUC__
#  define HARDWARE_SUPPORT
#  endif
#endif

typedef struct
{
    char* data;
    uint32_t size;
} Memory;

typedef struct
{
    void** functions;
    uint32_t size;
} Table;

typedef struct {
    uint64_t low;
    uint64_t high;
} v128_t;

#ifdef HARDWARE_SUPPORT
typedef char     cx16_t  __attribute__ ((vector_size (16)));
typedef int8_t   i8x16_t __attribute__ ((vector_size (16)));
typedef uint8_t  u8x16_t __attribute__ ((vector_size (16)));
typedef int16_t  i16x8_t __attribute__ ((vector_size (16)));
typedef uint16_t u16x8_t __attribute__ ((vector_size (16)));
typedef int32_t  i32x4_t __attribute__ ((vector_size (16)));
typedef uint32_t u32x4_t __attribute__ ((vector_size (16)));
typedef int64_t  i64x2_t __attribute__ ((vector_size (16)));
typedef uint64_t u64x2_t __attribute__ ((vector_size (16)));

typedef float    f32x4_t __attribute__ ((vector_size (16)));
typedef double   f64x2_t __attribute__ ((vector_size (16)));
#endif

typedef union
{
    v128_t   v128;
    int8_t   i8[16];
    uint8_t  u8[16];
    int16_t  i16[8];
    uint16_t u16[8];
    int32_t  i32[4];
    uint32_t u32[4];
    int64_t  i64[2];
    uint64_t u64[2];
    float    f32[4];
    double   f64[2];
#ifdef HARDWARE_SUPPORT
    cx16_t  cx16;
    i8x16_t i8x16;
    u8x16_t u8x16;
    i16x8_t i16x8;
    u16x8_t u16x8;
    i32x4_t i32x4;
    u32x4_t u32x4;
    i64x2_t i64x2;
    u64x2_t u64x2;
    f32x4_t f32x4;
    f64x2_t f64x2;
#endif

} v128_u;

extern void initializeMemory(Memory*, uint32_t min, uint32_t max);
extern void initializeTable(Table*, uint32_t min, uint32_t max);
extern uint32_t growMemory(Memory*, uint32_t size);

int32_t reinterpretI32F32(float value);
int64_t reinterpretI64F64(double value);
float reinterpretF32I32(int32_t value);
double reinterpretF64I64(int64_t value);

#ifdef HARDWARE_SUPPORT
#define clz32(value) ( ((value) == 0) ? 32 : __builtin_clz(value))
#define clz64(value) ( ((value) == 0) ? 64 : __builtin_clzl(value))
#define ctz32(value) ( ((value) == 0) ? 32 : __builtin_ctz(value))
#define ctz64(value) ( ((value) == 0) ? 64 : __builtin_ctzl(value))
#define popcnt32(value) __builtin_popcount(value)
#define popcnt64(value) __builtin_popcountll(value)
#else
uint32_t clz32(uint32_t value);
uint32_t clz64(uint64_t value);
uint32_t ctz32(uint32_t value);
uint32_t ctz64(uint64_t value);
uint32_t popcnt32(uint32_t value);
uint32_t popcnt64(uint64_t value);
#endif

#define rotl32(value, count) (((uint32_t)(value) << (count)) | ((uint32_t)(value) >> (32 - (count))))
#define rotr32(value, count) (((uint32_t)(value) >> (count)) | ((uint32_t)(value) << (32 - (count))))
#define rotl64(value, count) (((uint64_t)(value) << (count)) | ((uint64_t)(value) >> (64 - (count))))
#define rotr64(value, count) (((uint64_t)(value) >> (count)) | ((uint64_t)(value) << (64 - (count))))

int32_t satI32F32(float f);
uint32_t satU32F32(float f);
int32_t satI32F64(double f);
uint32_t satU32F64(double f);
int64_t satI64F32(float f);
uint64_t satU64F32(float f);
int64_t satI64F64(double f);
uint64_t satU64F64(double f);

#define MAX_VALUE(v1,v2) ((v1 < v2) ? v2 : v1)
#define MIN_VALUE(v1,v2) ((v1 < v2) ? v1 : v2)
#define AVGR(v1, v2) (v1 + v2 + 1) / 2
#define ABS_VALUE(v) ((v < 0) ? -v : v)
#define U(v) ((v128_u)(v))
#define V(v) U(v).v128

/*
 * 'simdFunctions.h' is generated.
 * It contains macros for hardware simd support like:
 *
 *   #define v128Addi8x16(v1,v2) V(U(v1).i8x16 + U(v2).i8x16)
 * 
 * It contains function declarations for software simd support like:
 *
 *   v128Addi8x16(v128_t v1, v128_t v2);
 * 
 */

#include "simdFunctions.h"

#define v128AndNoti64x2(v1,v2) v128Andi64x2(v1, v128Noti64x2(v2))

#endif
