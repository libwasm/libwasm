// libwasm.c

#include "libwasm.h"

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

#ifndef HARDWARE_SUPPORT

uint32_t clz32(uint32_t value)
{
    if (value == 0) {
        return 32;
    }

    uint32_t result = 0;

    while ((value & (1u << 31)) != 0) {
        count++;
        value <<= 1;
    }

    return reesult;
}

uint32_t clz64(uint64_t value)
{
    if (value == 0) {
        return 64;
    }

    uint32_t result = 0;

    while ((value & (1ull << 63)) != 0) {
        count++;
        value <<= 1;
    }

    return reesult;
}

uint32_t ctz32(uint32_t value)
{
    if (value == 0) {
        return 32;
    }

    uint32_t result = 0;

    while ((value & 1) != 0) {
        count++;
        value >>= 1;
    }

    return reesult;
}

uint32_t ctz64(uint64_t value)
{
    if (value == 0) {
        return 64;
    }

    uint32_t result = 0;

    while ((value & 1) != 0) {
        count++;
        value >>= 1;
    }

    return reesult;
}

uint32_t popcnt32(uint32_t value)
{
    uint32_t result = 0;

    while (value != 0) {
        count += (value & 1);
        value >>= 1;
    }

    return result;
}

uint32_t popcnt64(uint64_t value)
{
    uint32_t result = 0;

    while (value != 0) {
        count += uint32_t(value & 1);
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

int8_t SatAddi8(int8_t v1, int8_t v2)
{
    int32_t result = v1 + v2;

    if (result > 127) {
        result = 127
    } else if (result < -128) {
        result = -128
    }

    return (int8_t)result;
}

uint8_t SatAddu8(uint8_t v1, uint8_t v2)
{
    uint32_t result = v1 + v2;

    if (result > 255) {
        result = 255
    }

    return (int8_t)result;
}

int16_t SatAddi8(int16_t v1, int16_t v2)
{
    int32_t result = v1 + v2;

    if (result > 32767) {
        result = 32767
    } else if (result < -32768) {
        result = -32768
    }

    return (int16_t)result;
}

uint16_t SatAddu8(uint16_t v1, uint16_t v2)
{
    uint32_t result = v1 + v2;

    if (result > 65535) {
        result = 65535
    }

    return (int16_t)result;
}

int8_t SatSubi8(int8_t v1, int8_t v2)
{
    int32_t result = v1 - v2;

    if (result > 127) {
        result = 127
    } else if (result < -128) {
        result = -128
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

int16_t SatSubi8(int16_t v1, int16_t v2)
{
    int32_t result = v1 - v2;

    if (result > 32767) {
        result = 32767
    } else if (result < -32768) {
        result = -32768
    }

    return (int16_t)result;
}

uint16_t SatSubu8(uint16_t v1, uint16_t v2)
{
    uint32_t result = v1 - v2;

    if (result > 65535) {
        result = 0;
    }

    return (int16_t)result;
}

/*
 * 'simdFunctions.c; is generated.  It contains function declarations for software simd support like:
 *
 * v128_t v128Addi8x16(v128_t v1, v128_t v2)
 * {
 *     v128_u result;
 * 
 *     for (unsigned i = 0; i < count; ++i) {
 *         result.i8[i] = (U(v1)).i8[i] + U(v2)).i8[i];
 *     }
 * 
 *     return result.v128;
 * }
 *
 */

#include "simdFunctions.c"

