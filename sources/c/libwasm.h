// libwasm.h

#ifndef LIBWASM_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOHARDWARE_SUPPORT
#  ifdef __GNUC__
#  define HARDWARE_SUPPORT
#  endif
#endif

#define memoryPageSize 65536

typedef struct
{
    char* data;
    uint32_t pageCount;
    uint32_t maxPageCount;
} Memory;

typedef struct
{
    void** data;
    uint32_t elemntCount;
    uint32_t maxElementCount;
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
extern uint32_t growMemory(Memory*, uint32_t size);
extern void initializeTable(Table*, uint32_t min, uint32_t max);

int32_t reinterpretI32F32(float value);
int64_t reinterpretI64F64(double value);
float reinterpretF32I32(int32_t value);
double reinterpretF64I64(int64_t value);

#ifdef HARDWARE_SUPPORT
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

uint32_t rotl32(uint32_t value, uint32_t count);
uint32_t rotr32(uint32_t value, uint32_t count);
uint64_t rotl64(uint64_t value, uint32_t count);
uint64_t rotr64(uint64_t value, uint32_t count);

float nanF32(uint32_t x);
double nanF64(uint64_t x);

float minF32(float v1, float v2);
double minF64(double v1, double v2);
float maxF32(float v1, float v2);
double maxF64(double v1, double v2);

int32_t satI32F32(float f);
uint32_t satU32F32(float f);
int32_t satI32F64(double f);
uint32_t satU32F64(double f);
int64_t satI64F32(float f);
uint64_t satU64F32(float f);
int64_t satI64F64(double f);
uint64_t satU64F64(double f);
int8_t satI8I16(int16_t v);
uint8_t satU8I16(int16_t v);
int16_t satI16I32(int32_t v);
uint16_t satU16I32(int32_t v);

v128_t satI32x4F32x4(v128_t f);
v128_t satU32x4F32x4(v128_t f);
v128_t convertF32x4I32x4(v128_t i);
v128_t convertF32x4U32x4(v128_t i);
v128_t narrowI8x16I16x8(v128_t v1, v128_t v2);
v128_t narrowU8x16I16x8(v128_t v1, v128_t v2);
v128_t narrowI16x8I32x4(v128_t v1, v128_t v2);
v128_t narrowU16x8I32x4(v128_t v1, v128_t v2);

#define V(v) U(v).v128

#define v128Bitselect(v1,v2,v3) \
    v128Ori64x2(v128Andi64x2(v1, v3), v128Andi64x2(v2, v128Noti64x2(v3)))

#ifdef HARDWARE_SUPPORT
#define v128Shufflei8x16(v1,v2,v3) \
    V(__builtin_shuffle(U(v1).i8x16, U(v2).i8x16,  U(v3).i8x16))
#else
v128_t v128Shufflei8x16(v128_t v1, v128_t v2, v128_t v3);
#endif

v128_t v128Swizzlei8x16(v128_t v1, v128_t v2);

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

#ifdef __cplusplus
}
#endif

#endif
