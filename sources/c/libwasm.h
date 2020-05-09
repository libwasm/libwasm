// libwasm.h

#include <stdint.h>

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

