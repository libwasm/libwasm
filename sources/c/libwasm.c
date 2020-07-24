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

int8_t loadI8(Memory* memory, uint64_t offset)
{
    return *(int8_t*)(memory->data + offset);
}

uint8_t loadU8(Memory* memory, uint64_t offset)
{
    return loadI8(memory, offset);
}

int16_t loadI16(Memory* memory, uint64_t offset)
{
    uint8_t* p = (uint8_t*)(memory->data + offset);

    return p[0] | (p[1] << 8);
}

uint16_t loadU16(Memory* memory, uint64_t offset)
{
    return loadI16(memory, offset);
}

int32_t loadI32(Memory* memory, uint64_t offset)
{
    uint8_t* p = (uint8_t*)(memory->data + offset);

    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

uint32_t loadU32(Memory* memory, uint64_t offset)
{
    return loadI32(memory, offset);
}

int64_t loadI64(Memory* memory, uint64_t offset)
{
    return loadU32(memory, offset) | ((uint64_t)loadU32(memory, offset + 4) << 32);
}

uint64_t loadU64(Memory* memory, uint64_t offset)
{
    return loadI64(memory, offset);
}

float loadF32(Memory* memory, uint64_t offset)
{
    union
    {
        int32_t i;
        float f;
    } u;

    u.i = loadI32(memory, offset);
    return u.f;
}

double loadF64(Memory* memory, uint64_t offset)
{
    union
    {
        int64_t i;
        double f;
    } u;

    u.i = loadI64(memory, offset);
    return u.f;
}

v128_t loadV128(Memory* memory, uint64_t offset)
{
    union
    {
        int64_t i[2];
        v128_t v;
    } u;

    u.i[0] = loadI64(memory, offset);
    u.i[1] = loadI64(memory, offset + 8);
    return u.v;
}

int32_t loadI32U8(Memory* memory, uint64_t offset)
{
    return loadU8(memory, offset);
}

int32_t loadI32I8(Memory* memory, uint64_t offset)
{
    return loadI8(memory, offset);
}

int32_t loadI32U16(Memory* memory, uint64_t offset)
{
    return loadU16(memory, offset);
}

int32_t loadI32I16(Memory* memory, uint64_t offset)
{
    return loadI16(memory, offset);
}

int64_t loadI64U8(Memory* memory, uint64_t offset)
{
    return loadU8(memory, offset);
}

int64_t loadI64I8(Memory* memory, uint64_t offset)
{
    return loadI8(memory, offset);
}

int64_t loadI64U16(Memory* memory, uint64_t offset)
{
    return loadU16(memory, offset);
}

int64_t loadI64I16(Memory* memory, uint64_t offset)
{
    return loadI16(memory, offset);
}

int64_t loadI64U32(Memory* memory, uint64_t offset)
{
    return loadU32(memory, offset);
}

int64_t loadI64I32(Memory* memory, uint64_t offset)
{
    return loadI32(memory, offset);
}

void storeI32(Memory* memory, uint64_t offset, int32_t value)
{
    uint8_t* p = (uint8_t*)(memory->data + offset);

    p[0] = value;
    p[1] = value >> 8;
    p[2] = value >> 16;
    p[3] = value >> 24;
}

void storeI64(Memory* memory, uint64_t offset, int64_t value)
{
    storeI32(memory, offset, (int32_t)value);
    storeI32(memory, offset + 4, (int32_t)(value >> 32));
}

void storeF32(Memory* memory, uint64_t offset, float value)
{
    union
    {
        int32_t i;
        float f;
    } u;

    u.f = value;
    storeI32(memory, offset, u.i);
}

void storeF64(Memory* memory, uint64_t offset, double value)
{
    union
    {
        int64_t i;
        double f;
    } u;

    u.f = value;
    storeI64(memory, offset, u.i);
}

void storeV128(Memory* memory, uint64_t offset, v128_t value)
{
    union
    {
        int64_t i[2];
        v128_t v;
    } u;

    u.v = value;
    storeI64(memory, offset, u.i[0]);
    storeI64(memory, offset + 8, u.i[1]);
}

void storeI32I8(Memory* memory, uint64_t offset, int32_t value)
{
    *(int8_t*)(memory->data + offset) = (int8_t)value;
}

void storeI32I16(Memory* memory, uint64_t offset, int32_t value)
{
    uint8_t* p = (uint8_t*)(memory->data + offset);

    p[0] = value;
    p[1] = value >> 8;
}

void storeI64I8(Memory* memory, uint64_t offset, int64_t value)
{
    *(int8_t*)(memory->data + offset) = (int8_t)value;
}

void storeI64I16(Memory* memory, uint64_t offset, int64_t value)
{
    uint8_t* p = (uint8_t*)(memory->data + offset);

    p[0] = (int32_t)value;
    p[1] = (int32_t)value >> 8;
}

void storeI64I32(Memory* memory, uint64_t offset, int64_t value)
{
    storeI32(memory, offset, (int32_t)value);
}

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
    uint64_t pageCount64 = (uint64_t)memory->pageCount + size;

    if (pageCount64 == 0) {
        return 0;
    }

    if (pageCount64 < memory->pageCount || pageCount64 > memory->maxPageCount) {
        return -1;
    }

    uint32_t pageCount = (uint32_t)pageCount64;

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

uint32_t growTable(Table* table, uint32_t size)
{
    uint64_t elementCount64 = (uint64_t)table->elementCount + size;

    if (elementCount64 == 0) {
        return 0;
    }

    if (elementCount64 < table->elementCount || elementCount64 > table->maxElementCount) {
        return -1;
    }

    uint32_t elementCount = (uint32_t)elementCount64;

    void** data = realloc(table->data, elementCount * sizeof(void*));

    if (data == NULL) {
        return -1;
    }

    uint32_t result = table->elementCount;

    memset(data + result, 0, size * sizeof(void*));
    table->elementCount = elementCount;
    table->data = data;

    return result;
}

void fillMemory(Memory* memory, uint32_t to, uint32_t value, uint32_t size)
{
    memset(memory->data + to, value, size);
}

extern void copyMemory(Memory* dst, Memory* src, uint32_t to, uint32_t from, uint32_t size)
{
    memmove(dst->data + to, src->data + from, size);
}

void initMemory(Memory* memory, const char* data, uint32_t to, uint32_t from,
        uint32_t size)
{
    memcpy(memory->data + to, data + from, size);
}

void initializeTable(Table* table, uint32_t min, uint32_t max)
{
    table->elementCount = min;
    table->maxElementCount = max;

    if (min == 0) {
        table->data = NULL;
    } else {
        table->data = calloc(min, sizeof(void*));
    }
}

void fillTable(Table* table, uint32_t to, void* value, uint32_t size)
{
    while (size-- > 0) {
        table->data[to++] = value;
    }
}

void copyTable(Table* dst, Table*src, uint32_t to, uint32_t from, uint32_t size)
{
    memmove(dst->data + to, src->data + from, size * sizeof(void*));
}

void initTable(Table* table, const void** data, uint32_t to, uint32_t from,
        uint32_t size)
{
    memcpy(table->data + to, data + from, size * sizeof(void*));
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

v128_t narrowU16x8I32x4(v128_t v1, v128_t v2)
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

v128_t v128Shufflei8x16(v128_t v1, v128_t v2, v128_t v3)
{
    v128_u result;
    v128_u v1u = U(v1);
    v128_u v2u = U(v2);
    v128_u v3u = U(v3);

    for (unsigned i = 0; i < 16; ++i) {
        uint32_t index = v3u.u8[i];

        if (index >= 0 && index < 16) {
            result.i8[i] =  v1u.i8[index];
        } else {
            result.i8[i] =  v2u.i8[index - 16];
        }
    }

    return result.v128;
}

v128_t v128Swizzlei8x16(v128_t v1, v128_t v2)
{
    v128_u result;
    v128_u v1u = U(v1);
    v128_u v2u = U(v2);

    for (unsigned i = 0; i < 16; ++i) {
        int32_t index = v2u.i8[i];

        if (index >= 0 && index < 16) {
            result.i8[i] =  v1u.i8[index];
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

