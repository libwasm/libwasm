// Encodings.h

#ifndef OPCODES_H
#define OPCODES_H

#include "common.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>

enum class ParameterEncoding
{
    none,
    i32,
    i64,
    f32,
    f64,
    block,
    idx,
    localIdx,
    globalIdx,
    functionIdx,
    labelIdx,
    table,
    memory,
    memory0,
    indirect,
    max = indirect
};

class Opcode
{
    public:
        enum : uint8_t
        {
            unreachable = 0x00,
            nop = 0x01,
            block = 0x02,
            loop = 0x03,
            if_ = 0x04,
            else_ = 0x05,
            end = 0x0B,
            br = 0x0C,
            br_if = 0x0D,
            br_table = 0x0E,
            return_ = 0x0F,
            call = 0x10,
            call_indirect = 0x11,
            drop = 0x1A,
            select = 0x1B,
            local__get = 0x20,
            local__set = 0x21,
            local__tee = 0x22,
            global__get = 0x23,
            global__set = 0x24,
            i32__load = 0x28,
            i64__load = 0x29,
            f32__load = 0x2A,
            f64__load = 0x2B,
            i32__load8_s = 0x2C,
            i32__load8_u = 0x2D,
            i32__load16_s = 0x2E,
            i32__load16_u = 0x2F,
            i64__load8_s = 0x30,
            i64__load8_u = 0x31,
            i64__load16_s = 0x32,
            i64__load16_u = 0x33,
            i64__load32_s = 0x34,
            i64__load32_u = 0x35,
            i32__store = 0x36,
            i64__store = 0x37,
            f32__store = 0x38,
            f64__store = 0x39,
            i32__store8 = 0x3A,
            i32__store16 = 0x3B,
            i64__store8 = 0x3C,
            i64__store16 = 0x3D,
            i64__store32 = 0x3E,
            memory__size = 0x3F,
            memory__grow = 0x40,
            i32__const = 0x41,
            i64__const = 0x42,
            f32__const = 0x43,
            f64__const = 0x44,
            i32__eqz = 0x45,
            i32__eq = 0x46,
            i32__ne = 0x47,
            i32__lt_s = 0x48,
            i32__lt_u = 0x49,
            i32__gt_s = 0x4A,
            i32__gt_u = 0x4B,
            i32__le_s = 0x4C,
            i32__le_u = 0x4D,
            i32__ge_s = 0x4E,
            i32__ge_u = 0x4F,
            i64__eqz = 0x50,
            i64__eq = 0x51,
            i64__ne = 0x52,
            i64__lt_s = 0x53,
            i64__lt_u = 0x54,
            i64__gt_s = 0x55,
            i64__gt_u = 0x56,
            i64__le_s = 0x57,
            i64__le_u = 0x58,
            i64__ge_s = 0x59,
            i64__ge_u = 0x5A,
            f32__eq = 0x5B,
            f32__ne = 0x5C,
            f32__lt = 0x5D,
            f32__gt = 0x5E,
            f32__le = 0x5F,
            f32__ge = 0x60,
            f64__eq = 0x61,
            f64__ne = 0x62,
            f64__lt = 0x63,
            f64__gt = 0x64,
            f64__le = 0x65,
            f64__ge = 0x66,
            i32__clz = 0x67,
            i32__ctz = 0x68,
            i32__popcnt = 0x69,
            i32__add = 0x6A,
            i32__sub = 0x6B,
            i32__mul = 0x6C,
            i32__div_s = 0x6D,
            i32__div_u = 0x6E,
            i32__rem_s = 0x6F,
            i32__rem_u = 0x70,
            i32__and = 0x71,
            i32__or = 0x72,
            i32__xor = 0x73,
            i32__shl = 0x74,
            i32__shr_s = 0x75,
            i32__shr_u = 0x76,
            i32__rotl = 0x77,
            i32__rotr = 0x78,
            i64__clz = 0x79,
            i64__ctz = 0x7A,
            i64__popcnt = 0x7B,
            i64__add = 0x7C,
            i64__sub = 0x7D,
            i64__mul = 0x7E,
            i64__div_s = 0x7F,
            i64__div_u = 0x80,
            i64__rem_s = 0x81,
            i64__rem_u = 0x82,
            i64__and = 0x83,
            i64__or = 0x84,
            i64__xor = 0x85,
            i64__shl = 0x86,
            i64__shr_s = 0x87,
            i64__shr_u = 0x88,
            i64__rotl = 0x89,
            i64__rotr = 0x8A,
            f32__abs = 0x8B,
            f32__neg = 0x8C,
            f32__ceil = 0x8D,
            f32__floor = 0x8E,
            f32__trunc = 0x8F,
            f32__nearest = 0x90,
            f32__sqrt = 0x91,
            f32__add = 0x92,
            f32__sub = 0x93,
            f32__mul = 0x94,
            f32__div = 0x95,
            f32__min = 0x96,
            f32__max = 0x97,
            f32__copysign = 0x98,
            f64__abs = 0x99,
            f64__neg = 0x9A,
            f64__ceil = 0x9B,
            f64__floor = 0x9C,
            f64__trunc = 0x9D,
            f64__nearest = 0x9E,
            f64__sqrt = 0x9F,
            f64__add = 0xA0,
            f64__sub = 0xA1,
            f64__mul = 0xA2,
            f64__div = 0xA3,
            f64__min = 0xA4,
            f64__max = 0xA5,
            f64__copysign = 0xA6,
            i32__wrap_i64 = 0xA7,
            i32__trunc_f32_s = 0xA8,
            i32__trunc_f32_u = 0xA9,
            i32__trunc_f64_s = 0xAA,
            i32__trunc_f64_u = 0xAB,
            i64__extend_i32_s = 0xAC,
            i64__extend_i32_u = 0xAD,
            i64__trunc_f32_s = 0xAE,
            i64__trunc_f32_u = 0xAF,
            i64__trunc_f64_s = 0xB0,
            i64__trunc_f64_u = 0xB1,
            f32__convert_i32_s = 0xB2,
            f32__convert_i32_u = 0xB3,
            f32__convert_i64_s = 0xB4,
            f32__convert_i64_u = 0xB5,
            f32__demote_f64 = 0xB6,
            f64__convert_i32_s = 0xB7,
            f64__convert_i32_u = 0xB8,
            f64__convert_i64_s = 0xB9,
            f64__convert_i64_u = 0xBA,
            f64__promote_f32 = 0xBB,
            i32__reinterpret_f32 = 0xBC,
            i64__reinterpret_f64 = 0xBD,
            f32__reinterpret_i32 = 0xBE,
            f64__reinterpret_i64 = 0xBF,

            max = f64__reinterpret_i64
        };

        Opcode() = default;
        explicit Opcode(uint8_t v)
          : value(v)
        {
        }

        std::string_view getName() const;
        ParameterEncoding getParameterEncoding() const;
        uint32_t getAlign() const;
        bool isfloat() const;
        bool isValid() const;
        
        bool operator==(uint8_t v) const
        {
            return value == v;
        }

        bool operator!=(uint8_t v) const
        {
            return value != v;
        }

        explicit operator uint8_t() const
        {
            return value;
        }

        static std::optional<Opcode> fromString(std::string_view name);

    private:
        struct Info
        {
            ParameterEncoding encoding;
            std::string_view name;
            uint32_t align = 0;
            bool mFloat = false;
        };

        struct Entry
        {
            std::string_view name;
            uint8_t opcode;
        };

        static Info info[size_t(Opcode::max) + 1];
        static Entry map[];
        uint8_t value = 0;
};

inline bool operator==(uint8_t v, const Opcode& opcode)
{
    return opcode == v;
}

inline bool operator!=(uint8_t v, const Opcode& opcode)
{
    return opcode != v;
}

inline std::ostream& operator<<(std::ostream& os, Opcode opcode)
{
    return os << opcode.getName();
}

class SectionType
{
    public:
        enum : uint8_t
        {
            custom,
            type,
            import,
            function,
            table,
            memory,
            global,
            export_,
            start,
            element,
            code,
            data,
            dataCount,
            max = dataCount
        };

        bool isValid() const;
        std::string_view getName() const;
        
        bool operator==(const SectionType& v) const
        {
            return value == v.value;
        }

        bool operator!=(const SectionType& v) const
        {
            return value != v.value;
        }

        bool operator==(uint8_t v) const
        {
            return value == v;
        }

        bool operator!=(uint8_t v) const
        {
            return value != v;
        }

        SectionType() = default;
        SectionType(const SectionType& v)
          : value(v.value)
        {
        }

        SectionType(uint8_t v)
          : value(v)
        {
        }

        explicit operator uint8_t() const
        {
            return value;
        }

    private:
        uint8_t value = max + 1;
};

inline bool operator==(uint8_t v, const SectionType& type)
{
    return type == v;
}

inline bool operator!=(uint8_t v, const SectionType& type)
{
    return type != v;
}

inline std::ostream& operator<<(std::ostream& os, SectionType type)
{
    return os << type.getName();
}

class ValueType
{
    public:
        enum : int32_t
        {
            i32 = -0x01,      // 0x7f
            i64 = -0x02,      // 0x7e
            f32 = -0x03,      // 0x7d
            f64 = -0x04,      // 0x7c
            v128 = -0x05,     // 0x7b
            funcref = -0x10,  // 0x70
            anyref = -0x11,   // 0x6f
            exnref = -0x18,   // 0x68
            func = -0x20,     // 0x60
            void_ = -0x40,    // 0x40
        };

        bool isValid() const;
        std::string_view getName() const;
        
        bool operator==(int32_t v) const
        {
            return value == v;
        }

        bool operator!=(int32_t v) const
        {
            return value != v;
        }

        ValueType() = default;
        ValueType(int32_t v)
          : value(v)
        {
        }

        bool operator==(const ValueType& other) const
        {
            return value == other.value;
        }

        bool operator!=(const ValueType& other) const
        {
            return value != other.value;
        }

        explicit operator int32_t()
        {
            return value;
        }

        static std::optional<ValueType> getEncoding(std::string_view v);

    private:
        int32_t value;
        
};

inline bool operator==(int32_t v, const ValueType& type)
{
    return type == v;
}

inline bool operator!=(int32_t v, const ValueType& type)
{
    return type != v;
}

inline std::ostream& operator<<(std::ostream& os, ValueType type)
{
    return os << type.getName();
}

class ExternalKind
{
    public:
        enum : uint8_t
        {
            function,
            table,
            memory,
            global,
            event,
            max = event
        };

        bool isValid() const;
        std::string_view getName() const;
        
        bool operator==(uint8_t v) const
        {
            return value == v;
        }

        bool operator!=(uint8_t v) const
        {
            return value != v;
        }

        ExternalKind() = default;
        ExternalKind(uint8_t v)
          : value(v)
        {
        }

        explicit operator uint8_t() const
        {
            return value;
        }

        static std::optional<ExternalKind> getEncoding(std::string_view name);

    private:
        uint8_t value = max + 1;
};

inline bool operator==(uint8_t v, const ExternalKind& type)
{
    return type == v;
}

inline bool operator!=(uint8_t v, const ExternalKind& type)
{
    return type != v;
}

inline std::ostream& operator<<(std::ostream& os, ExternalKind value)
{
    return os << value.getName();
}

enum class LimitKind : uint8_t
{
    onlyMin,
    hasMax
};

enum class Mut : uint8_t
{
    const_,
    var
};

enum class RelocationType : uint8_t
{
    functionIndexLeb,
    tableIndexSleb,
    tableIndexI32,
    memoryAddrLeb,
    memoryAddrSleb,
    memoryAddrI32,
    typeIndexLeb,
    globaLIndexLeb,
    functionOffsetI32,
    sectionOffsetI32,
    eventIndexLeb,
    memoryAddrRelSleb,
    tableIndexRelSleb
};

std::ostream& operator<<(std::ostream& os, RelocationType type);
bool hasAddend(RelocationType type);

enum class LinkingType : uint8_t
{
    segmentInfo = 5,
    initFuncs,
    comDatInfo,
    symbolTable,
};

std::ostream& operator<<(std::ostream& os, LinkingType type);

enum class ComdatSymKind : uint8_t
{
    data,
    function,
    global,
    event,
    table
};

std::ostream& operator<<(std::ostream& os, ComdatSymKind type);

enum class SymbolKind : uint8_t
{
    function,
    data,
    global,
    section,
    event,
    table
};

std::ostream& operator<<(std::ostream& os, SymbolKind type);

enum class SymbolFlags : uint32_t
{
    none = 0,
    weak = 0x1,
    local = 0x2,
    hidden = 0x4,
    undefined = 0x10,
    exported = 0x20,
    explicitName = 0x40,
    noStrip = 0x80
};

struct Limits
{
    LimitKind kind = LimitKind::onlyMin;
    uint32_t min = 0;
    uint32_t max = 0;

    void show(std::ostream& os);
    void generate(std::ostream& os);
};

#endif
