// libwasm.h

#include <stdint.h>
#include <math.h>

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

typedef union
{
    int8_t  v16x8[16];
    int16_t v8x16[8];
    int32_t v4x32[4];
    int64_t v2x64[2];

} V128;

extern void initializeMemory(Memory*, uint32_t min, uint32_t max);
extern void initializeTable(Table*, uint32_t min, uint32_t max);
extern uint32_t growMemory(Memory*, uint32_t size);

inline uint32_t reinterpretI32F32(float value)
{
    return *(uint32_t*)&value;
}

inline uint64_t reinterpretI64F64(double value)
{
    return *(uint64_t*)&value;
}

inline float reinterpretF32I32(uint32_t value)
{
    return *(float*)&value;
}

inline double reinterpretF64I64(uint64_t value)
{
    return *(double*)&value;
}

#ifdef __GNUC__
inline uint32_t clz32(uint32_t value)
{
    return (value == 0) ? 32 : __builtin_clz(value);
}

inline uint32_t clz64(uint64_t value)
{
    return (value == 0) ? 64 : __builtin_clzll(value);
}

inline uint32_t ctz32(uint32_t value)
{
    return (value == 0) ? 32 : __builtin_ctz(value);
}

inline uint32_t ctz64(uint64_t value)
{
    return (value == 0) ? 64 : __builtin_ctzll(value);
}

inline uint32_t popcnt32(uint32_t value)
{
    return __builtin_popcount(value);
}

inline uint32_t popcnt64(uint64_t value)
{
    return __builtin_popcountll(value);
}
#else
inline uint32_t clz32(uint32_t value)
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

inline uint32_t clz64(uint64_t value)
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

inline uint32_t ctz32(uint32_t value)
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

inline uint32_t ctz64(uint64_t value)
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

inline uint32_t popcnt32(uint32_t value)
{
    uint32_t result = 0;

    while (value != 0) {
        count += (value & 1);
        value >>= 1;
    }

    return result;
}

inline uint32_t popcnt64(uint64_t value)
{
    uint32_t result = 0;

    while (value != 0) {
        count += uint32_t(value & 1);
        value >>= 1;
    }

    return result;
}
#endif

inline uint32_t rotl32(uint32_t value, uint32_t count)
{
    return ((value << count) | (value >> (32 - count)));
}

inline uint32_t rotr32(uint32_t value, uint32_t count)
{
    return ((value >> count) | (value << (32 - count)));
}

inline uint64_t rotl64(uint64_t value, uint64_t count)
{
    return ((value << count) | (value >> (64 - count)));
}

inline uint64_t rotr64(uint64_t value, uint64_t count)
{
    return ((value >> count) | (value << (64 - count)));
}

inline int32_t satI32F32(float f)
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

inline uint32_t satU32F32(float f)
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

inline int32_t satI32F64(double f)
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

inline uint32_t satU32F64(double f)
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

inline int64_t satI64F32(float f)
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

inline uint64_t satU64F32(float f)
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

inline int64_t satI64F64(double f)
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

inline uint64_t satU64F64(double f)
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

V128 makeV128(uint64_t v0, uint64_t v1)
{
    V128 result;

    result.v2x64[0] = v0;
    result.v2x64[1] = v1;

    return result;
}

