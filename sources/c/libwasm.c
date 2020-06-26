// libwasm.c

#include "libwasm.h"

#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_VALUE(v1,v2) ((v1 < v2) ? v2 : v1)
#define MIN_VALUE(v1,v2) ((v1 < v2) ? v1 : v2)
#define AVGR(v1, v2) (v1 + v2 + 1) / 2
#define ABS_VALUE(v) ((v < 0) ? -v : v)
#define U(v) ((v128_u)(v))

uint32_t rotl32(uint32_t value, uint32_t count)
{
    count %= 32;

    if (count == 0) {
        return value;
    }

    return (value << count) | (value >> (32 - count));
}

uint32_t rotr32(uint32_t value, uint32_t count)
{
    count %= 32;

    if (count == 0) {
        return value;
    }

    return (value >> count) | (value << (32 - count));
}

uint64_t rotl64(uint64_t value, uint32_t count)
{
    count %= 64;

    if (count == 0) {
        return value;
    }

    return (value << count) | (value >> (64 - count));
}

uint64_t rotr64(uint64_t value, uint32_t count)
{
    count %= 64;

    if (count == 0) {
        return value;
    }

    return (value >> count) | (value << (64 - count));
}

float nanF32(uint32_t x)
{
    uint32_t bits = 0x7f800000 | *(uint32_t*)&x;
    return *(float*)&bits;
}

double nanF64(uint64_t x)
{
    uint64_t bits = 0x7ff0000000000000ULL | *(uint64_t*)&x;
    return *(double*)&bits;
}

float minF32(float v1, float v2)
{
    if (isnan(v1) || isnan(v2)) {
        return NAN;
    } else if (v1 == 0 && v2 == 0) {
        return (*(int32_t*)&v1 < *(int32_t*)&v2) ? v1 : v2;
    } else {
        return (v1 < v2) ? v1 : v2;
    }
}

double minF64(double v1, double v2)
{
    if (isnan(v1) || isnan(v2)) {
        return NAN;
    } else if (v1 == 0 && v2 == 0) {
        return (*(int64_t*)&v1 < *(int64_t*)&v2) ? v1 : v2;
    } else {
        return (v1 < v2) ? v1 : v2;
    }
}

float maxF32(float v1, float v2)
{
    if (isnan(v1) || isnan(v2)) {
        return NAN;
    } else if (v1 == 0 && v2 == 0) {
        return (*(int32_t*)&v1 < *(int32_t*)&v2) ? v2 : v1;
    } else {
        return (v1 < v2) ? v2 : v1;
    }
}

double maxF64(double v1, double v2)
{
    if (isnan(v1) || isnan(v2)) {
        return NAN;
    } else if (v1 == 0 && v2 == 0) {
        return (*(int64_t*)&v1 < *(int64_t*)&v2) ? v2 : v1;
    } else {
        return (v1 < v2) ? v2 : v1;
    }
}

void initializeMemory(Memory* memory, uint32_t min, uint32_t max)
{
    memory->pageCount = min;
    memory->maxPageCount = max;

    if (min == 0) {
        memory->data = NULL;
    } else {
        memory->data = calloc(min, memoryPageSize);
    }
}

uint32_t growMemory(Memory* memory, uint32_t size)
{
    uint32_t pageCount = memory->pageCount + size;

    if (pageCount == 0) {
        return 0;
    }

    if (pageCount < memory->pageCount || pageCount > memory->maxPageCount) {
        return -1;
    }

    char* data = realloc(memory->data, pageCount * memoryPageSize);

    if (data == NULL) {
        return -1;
    }

    uint32_t result = memory->pageCount;

    memset(data + memory->pageCount * memoryPageSize, 0, size * memoryPageSize);
    memory->pageCount = pageCount;
    memory->data = data;

    return result;
}

void initializeTable(Table* table, uint32_t min, uint32_t max)
{
    table->elemntCount = min;
    table->maxElementCount = max;

    if (min == 0) {
        table->data = NULL;
    } else {
        table->data = calloc(min, sizeof(void*));
    }
}

int32_t reinterpretI32F32(float value)
{
    return *(int32_t*)&value;
}

int64_t reinterpretI64F64(double value)
{
    return *(int64_t*)&value;
}

float reinterpretF32I32(int32_t value)
{
    return *(float*)&value;
}

double reinterpretF64I64(int64_t value)
{
    return *(double*)&value;
}

#ifdef HARDWARE_SUPPORT
uint32_t clz32(uint32_t value)
{
    return (value == 0) ? 32 : __builtin_clz(value);
}

uint32_t clz64(uint64_t value)
{
    return (value == 0) ? 64 : __builtin_clzl(value);
}

uint32_t ctz32(uint32_t value)
{
    return (value == 0) ? 32 : __builtin_ctz(value);
}

uint32_t ctz64(uint64_t value)
{
    return (value == 0) ? 64 : __builtin_ctzl(value);
}

#else

uint32_t clz32(uint32_t value)
{
    if (value == 0) {
        return 32;
    }

    uint32_t result = 0;

    while ((value & (1u << 31)) != 0) {
        result++;
        value <<= 1;
    }

    return result;
}

uint32_t clz64(uint64_t value)
{
    if (value == 0) {
        return 64;
    }

    uint32_t result = 0;

    while ((value & (1ull << 63)) != 0) {
        result++;
        value <<= 1;
    }

    return result;
}

uint32_t ctz32(uint32_t value)
{
    if (value == 0) {
        return 32;
    }

    uint32_t result = 0;

    while ((value & 1) != 0) {
        result++;
        value >>= 1;
    }

    return result;
}

uint32_t ctz64(uint64_t value)
{
    if (value == 0) {
        return 64;
    }

    uint32_t result = 0;

    while ((value & 1) != 0) {
        result++;
        value >>= 1;
    }

    return result;
}

uint32_t popcnt32(uint32_t value)
{
    uint32_t result = 0;

    while (value != 0) {
        result += (value & 1);
        value >>= 1;
    }

    return result;
}

uint32_t popcnt64(uint64_t value)
{
    uint32_t result = 0;

    while (value != 0) {
        result += (uint32_t)(value & 1);
        value >>= 1;
    }

    return result;
}

#endif

int32_t satI32F32(float f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= -2147483648.0F) {
        return -2147483648;
    } else if (f >= 2147483647.0F) {
        return 2147483647;
    } else {
        return (int32_t)f;
    }
}

uint32_t satU32F32(float f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= 0.0F) {
        return 0;
    } else if (f >= 4294967295.0F) {
        return 4294967295;
    } else {
        return (uint32_t)f;
    }
}

int32_t satI32F64(double f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= -2147483648.0) {
        return -2147483648;
    } else if (f >= 2147483647.0) {
        return 2147483647;
    } else {
        return (int32_t)f;
    }
}

uint32_t satU32F64(double f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= 0.0) {
        return 0;
    } else if (f >= 4294967295.0) {
        return 4294967295;
    } else {
        return (uint32_t)f;
    }
}

int64_t satI64F32(float f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= -9223372036854775808.0F) {
        return 0x8000000000000000LL;
    } else if (f >= 9223372036854775807.0F) {
        return 9223372036854775807LL;
    } else {
        return (int64_t)f;
    }
}

uint64_t satU64F32(float f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= 0.0F) {
        return 0;
    } else if (f >= 18446744073709551615.0F) {
        return 18446744073709551615ULL;
    } else {
        return (uint64_t)f;
    }
}

int64_t satI64F64(double f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= -9223372036854775808.0) {
        return 0x8000000000000000LL;
    } else if (f >= 9223372036854775807.0) {
        return 9223372036854775807LL;
    } else {
        return (int64_t)f;
    }
}

uint64_t satU64F64(double f)
{
    if (isnan(f)) {
        return 0;
    } else if (f <= 0.0) {
        return 0;
    } else if (f >= 18446744073709551615.0) {
        return 18446744073709551615ULL;
    } else {
        return (uint64_t)f;
    }
}

v128_t satI32x4F32x4(v128_t f)
{
    v128_u result;

    for (uint32_t i = 0; i < 4; ++i) {
        result.i32[i] = satI32F32((U(f)).f32[i]);
    }

    return result.v128;
}

v128_t satU32x4F32x4(v128_t f)
{
    v128_u result;

    for (uint32_t i = 0; i < 4; ++i) {
        result.u32[i] = satU32F32((U(f)).f32[i]);
    }

    return result.v128;
}

v128_t convertF32x4I32x4(v128_t f)
{
    v128_u result;

    for (uint32_t i = 0; i < 4; ++i) {
        result.f32[i] = (U(f)).i32[i];
    }

    return result.v128;
}

v128_t convertF32x4U32x4(v128_t f)
{
    v128_u result;

    for (uint32_t i = 0; i < 4; ++i) {
        result.f32[i] = (U(f)).u32[i];
    }

    return result.v128;
}

int8_t satI8I16(int16_t v)
{
    if (v > 127) {
        return 127;
    } else if (v < -128) {
        return -128;
    } else {
        return (int8_t)v;
    }
}

uint8_t satU8I16(int16_t v)
{
    if (v > 255) {
        return 255;
    } else if (v < 0) {
        return 0;
    } else {
        return (uint8_t)v;
    }
}

int16_t satI16I32(int32_t v)
{
    if (v > 32767) {
        return 32767;
    } else if (v < -32768) {
        return -32768;
    } else {
        return (int16_t)v;
    }
}

uint16_t satU16I32(int32_t v)
{
    if (v > 65535) {
        return 65535;
    } else if (v < 0) {
        return 0;
    } else {
        return (uint16_t)v;
    }
}

v128_t narrowI8x16I16x8(v128_t v1, v128_t v2)
{
    v128_u result;

    for (int i = 0; i < 8; ++i) {
        result.i8[i] = satI8I16(U(v1).i16[i]);
        result.i8[i + 8] = satI8I16(U(v2).i16[i]);
    }

    return result.v128;
}

v128_t narrowU8x16I16x8(v128_t v1, v128_t v2)
{
    v128_u result;

    for (int i = 0; i < 8; ++i) {
        result.u8[i] = satU8I16(U(v1).i16[i]);
        result.i8[i + 8] = satU8I16(U(v2).i16[i]);
    }

    return result.v128;
}

v128_t narrowI16x8I32x4(v128_t v1, v128_t v2)
{
    v128_u result;

    for (int i = 0; i < 4; ++i) {
        result.i16[i] = satI16I32(U(v1).i32[i]);
        result.i16[i + 4] = satI16I32(U(v2).i32[i]);
    }

    return result.v128;
}

v128_t narrowUU6x8I32x4(v128_t v1, v128_t v2)
{
    v128_u result;

    for (int i = 0; i < 4; ++i) {
        result.u16[i] = satU16I32(U(v1).i32[i]);
        result.u16[i + 4] = satU16I32(U(v2).i32[i]);
    }

    return result.v128;
}

int8_t SatAddi8(int8_t v1, int8_t v2)
{
    int32_t result = v1 + v2;

    if (result > 127) {
        result = 127;
    } else if (result < -128) {
        result = -128;
    }

    return (int8_t)result;
}

uint8_t SatAddu8(uint8_t v1, uint8_t v2)
{
    uint32_t result = v1 + v2;

    if (result > 255) {
        result = 255;
    }

    return (int8_t)result;
}

int16_t SatAddi16(int16_t v1, int16_t v2)
{
    int32_t result = v1 + v2;

    if (result > 32767) {
        result = 32767;
    } else if (result < -32768) {
        result = -32768;
    }

    return (int16_t)result;
}

uint16_t SatAddu16(uint16_t v1, uint16_t v2)
{
    uint32_t result = v1 + v2;

    if (result > 65535) {
        result = 65535;
    }

    return (int16_t)result;
}

int8_t SatSubi8(int8_t v1, int8_t v2)
{
    int32_t result = v1 - v2;

    if (result > 127) {
        result = 127;
    } else if (result < -128) {
        result = -128;
    }

    return (int8_t)result;
}

uint8_t SatSubu8(uint8_t v1, uint8_t v2)
{
    uint32_t result = v1 - v2;

    if (result > 255) {
        result = 0;
    }

    return (int8_t)result;
}

int16_t SatSubi16(int16_t v1, int16_t v2)
{
    int32_t result = v1 - v2;

    if (result > 32767) {
        result = 32767;
    } else if (result < -32768) {
        result = -32768;
    }

    return (int16_t)result;
}

uint16_t SatSubu16(uint16_t v1, uint16_t v2)
{
    uint32_t result = v1 - v2;

    if (result > 65535) {
        result = 0;
    }

    return (int16_t)result;
}

#ifndef HARDWARE_SUPPORT
v128_t v128Shufflei8x16(v128_t v1, v128_t v2, v128_t v3)
{
    v128_u result;

    for (unsigned i = 0; i < 16; ++i) {
        uint32_t index = U(v3).i8[i];

        if (index < 16) {
            result.i8[i] =  U(v1).i8[i];
        } else {
            result.i8[i] =  U(v2).i8[index - 16];
        }
    }

    return result.v128;
}

#endif

v128_t v128Swizzlei8x16(v128_t v1, v128_t v2)
{
    v128_u result;

    for (unsigned i = 0; i < 16; ++i) {
        uint32_t index = U(v2).i8[i];

        if (index < 16) {
            result.i8[i] =  U(v1).i8[i];
        } else {
            result.i8[i] =  0;
        }
    }

    return result.v128;
}

/*
 * 'simdFunctions.c; is generated.  It contains function declarations for software simd support like:
 *
 * v128_t v128Addi8x16(v128_t v1, v128_t v2)
 * {
 *     v128_u result;
 * 
 *     for (unsigned i = 0; i < 16; ++i) {
 *         result.i8[i] = U(v1).i8[i] + U(v2).i8[i];
 *     }
 * 
 *     return result.v128;
 * }
 *
 */

#include "simdFunctions.c"

#ifdef __cplusplus
}
#endif

