// Encodings.h

#ifndef OPCODES_H
#define OPCODES_H

#include "common.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

enum class ParameterEncoding
{
    none,
    i8,
    i32,
    i64,
    f32,
    f64,
    v128,
    block,
    idx,
    localIdx,
    globalIdx,
    functionIdx,
    labelIdx,
    laneIdx2,
    laneIdx4,
    laneIdx8,
    laneIdx16,
    laneIdx32,
    shuffle,
    table,
    memory,
    memory0,
    indirect,
};

class OpcodePrefix
{
    public:
        enum : uint8_t
        {
            simd = 0xfd,
            thread = 0xfe,
        };

        OpcodePrefix() = default;

        explicit OpcodePrefix(uint8_t v)
          : value(v)
        {
        }

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

        bool isValid() const
        {
            return value == simd || value == thread;
        }

    private:
        uint8_t value = 0;
};

inline bool operator==(uint8_t v, const OpcodePrefix& event)
{
    return event == v;
}

inline bool operator!=(uint8_t v, const OpcodePrefix& event)
{
    return event != v;
}

enum class SignatureCode
{
    void_,
    f32_,
    f32__f32_f32,
    f32__f64,
    f32__i32,
    f32__i64,
    f32__v128,
    f64_,
    f64__f32,
    f64__f64_f64,
    f64__i32,
    f64__i64,
    f64__v128,
    i32_,
    i32__f32,
    i32__f32_f32,
    i32__f64,
    i32__f64_f64,
    i32__i32,
    i32__i32_i32,
    i32__i32_i32_i32,
    i32__i32_i32_i64,
    i32__i32_i64_i64,
    i32__i64,
    i32__i64_i64,
    i32__v128,
    i64_,
    i64__f32,
    i64__f64,
    i64__i32,
    i64__i32_i64,
    i64__i32_i64_i64,
    i64__i64,
    i64__i64_i64,
    i64__v128,
    v128_,
    v128__f32,
    v128__f64,
    v128__i32,
    v128__i64,
    v128__v128,
    v128__v128_f32,
    v128__v128_f64,
    v128__v128_i32,
    v128__v128_i64,
    v128__v128_v128,
    v128__v128_v128_v128,
    void__i32_f32,
    void__i32_f64,
    void__i32_i32,
    void__i32_i32_i32,
    void__i32_i64,
    void__i32_v128,
};

class Opcode
{
    public:
        enum : uint32_t
        {
            unreachable = 0x00,
            nop = 0x01,
            block = 0x02,
            loop = 0x03,
            if_ = 0x04,
            else_ = 0x05,
            try_ = 0x06,
            catch_ = 0x07,
            throw_ = 0x08,
            rethrow_ = 0x09,
            br_on_exn = 0x0a,
            end = 0x0b,
            br = 0x0c,
            br_if = 0x0d,
            br_table = 0x0e,
            return_ = 0x0f,
            call = 0x10,
            call_indirect = 0x11,
            return_call = 0x12,
            return_call_indirect = 0x13,
            drop = 0x1a,
            select = 0x1b,
            local__get = 0x20,
            local__set = 0x21,
            local__tee = 0x22,
            global__get = 0x23,
            global__set = 0x24,
            i32__load = 0x28,
            i64__load = 0x29,
            f32__load = 0x2a,
            f64__load = 0x2b,
            i32__load8_s = 0x2c,
            i32__load8_u = 0x2d,
            i32__load16_s = 0x2e,
            i32__load16_u = 0x2f,
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
            i32__store8 = 0x3a,
            i32__store16 = 0x3b,
            i64__store8 = 0x3c,
            i64__store16 = 0x3d,
            i64__store32 = 0x3e,
            memory__size = 0x3f,
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
            i32__gt_s = 0x4a,
            i32__gt_u = 0x4b,
            i32__le_s = 0x4c,
            i32__le_u = 0x4d,
            i32__ge_s = 0x4e,
            i32__ge_u = 0x4f,
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
            i64__ge_u = 0x5a,
            f32__eq = 0x5b,
            f32__ne = 0x5c,
            f32__lt = 0x5d,
            f32__gt = 0x5e,
            f32__le = 0x5f,
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
            i32__add = 0x6a,
            i32__sub = 0x6b,
            i32__mul = 0x6c,
            i32__div_s = 0x6d,
            i32__div_u = 0x6e,
            i32__rem_s = 0x6f,
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
            i64__ctz = 0x7a,
            i64__popcnt = 0x7b,
            i64__add = 0x7c,
            i64__sub = 0x7d,
            i64__mul = 0x7e,
            i64__div_s = 0x7f,
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
            i64__rotr = 0x8a,
            f32__abs = 0x8b,
            f32__neg = 0x8c,
            f32__ceil = 0x8d,
            f32__floor = 0x8e,
            f32__trunc = 0x8f,
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
            f64__neg = 0x9a,
            f64__ceil = 0x9b,
            f64__floor = 0x9c,
            f64__trunc = 0x9d,
            f64__nearest = 0x9e,
            f64__sqrt = 0x9f,
            f64__add = 0xa0,
            f64__sub = 0xa1,
            f64__mul = 0xa2,
            f64__div = 0xa3,
            f64__min = 0xa4,
            f64__max = 0xa5,
            f64__copysign = 0xa6,
            i32__wrap_i64 = 0xa7,
            i32__trunc_f32_s = 0xa8,
            i32__trunc_f32_u = 0xa9,
            i32__trunc_f64_s = 0xaa,
            i32__trunc_f64_u = 0xab,
            i64__extend_i32_s = 0xac,
            i64__extend_i32_u = 0xad,
            i64__trunc_f32_s = 0xae,
            i64__trunc_f32_u = 0xaf,
            i64__trunc_f64_s = 0xb0,
            i64__trunc_f64_u = 0xb1,
            f32__convert_i32_s = 0xb2,
            f32__convert_i32_u = 0xb3,
            f32__convert_i64_s = 0xb4,
            f32__convert_i64_u = 0xb5,
            f32__demote_f64 = 0xb6,
            f64__convert_i32_s = 0xb7,
            f64__convert_i32_u = 0xb8,
            f64__convert_i64_s = 0xb9,
            f64__convert_i64_u = 0xba,
            f64__promote_f32 = 0xbb,
            i32__reinterpret_f32 = 0xbc,
            i64__reinterpret_f64 = 0xbd,
            f32__reinterpret_i32 = 0xbe,
            f64__reinterpret_i64 = 0xbf,
            i32__extend8_s = 0xc0,
            i32__extend16_s = 0xc1,
            i64__extend8_s = 0xc2,
            i64__extend16_s = 0xc3,
            i64__extend32_s = 0xc4,
            alloca = 0xe0,
            br_unless = 0xe1,
            call_host = 0xe2,
            data = 0xe3,
            drop_keep = 0xe4,
            i32__trunc_sat_f32_s = 0x00,
            i32__trunc_sat_f32_u = 0x01,
            i32__trunc_sat_f64_s = 0x02,
            i32__trunc_sat_f64_u = 0x03,
            i64__trunc_sat_f32_s = 0x04,
            i64__trunc_sat_f32_u = 0x05,
            i64__trunc_sat_f64_s = 0x06,
            i64__trunc_sat_f64_u = 0x07,
            memory__init = 0x08,
            data__drop = 0x09,
            memory__copy = 0x0a,
            memory__fill = 0x0b,
            table__init = 0x0c,
            elem__drop = 0x0d,
            table__copy = 0x0e,
            table__get = 0x25,
            table__set = 0x26,
            table__grow = 0x0f,
            table__size = 0x10,
            table__fill = 0x11,
            ref__null = 0xd0,
            ref__is_null = 0xd1,
            ref__func = 0xd2,

            // SIMD
            v128__load = 0x00,
            v128__store = 0x01,
            v128__const = 0x02,
            i8x16__splat = 0x04,
            i8x16__extract_lane_s = 0x05,
            i8x16__extract_lane_u = 0x06,
            i8x16__replace_lane = 0x07,
            i16x8__splat = 0x08,
            i16x8__extract_lane_s = 0x09,
            i16x8__extract_lane_u = 0x0a,
            i16x8__replace_lane = 0x0b,
            i32x4__splat = 0x0c,
            i32x4__extract_lane = 0x0d,
            i32x4__replace_lane = 0x0e,
            i64x2__splat = 0x0f,
            i64x2__extract_lane = 0x10,
            i64x2__replace_lane = 0x11,
            f32x4__splat = 0x12,
            f32x4__extract_lane = 0x13,
            f32x4__replace_lane = 0x14,
            f64x2__splat = 0x15,
            f64x2__extract_lane = 0x16,
            f64x2__replace_lane = 0x17,
            i8x16__eq = 0x18,
            i8x16__ne = 0x19,
            i8x16__lt_s = 0x1a,
            i8x16__lt_u = 0x1b,
            i8x16__gt_s = 0x1c,
            i8x16__gt_u = 0x1d,
            i8x16__le_s = 0x1e,
            i8x16__le_u = 0x1f,
            i8x16__ge_s = 0x20,
            i8x16__ge_u = 0x21,
            i16x8__eq = 0x22,
            i16x8__ne = 0x23,
            i16x8__lt_s = 0x24,
            i16x8__lt_u = 0x25,
            i16x8__gt_s = 0x26,
            i16x8__gt_u = 0x27,
            i16x8__le_s = 0x28,
            i16x8__le_u = 0x29,
            i16x8__ge_s = 0x2a,
            i16x8__ge_u = 0x2b,
            i32x4__eq = 0x2c,
            i32x4__ne = 0x2d,
            i32x4__lt_s = 0x2e,
            i32x4__lt_u = 0x2f,
            i32x4__gt_s = 0x30,
            i32x4__gt_u = 0x31,
            i32x4__le_s = 0x32,
            i32x4__le_u = 0x33,
            i32x4__ge_s = 0x34,
            i32x4__ge_u = 0x35,
            f32x4__eq = 0x40,
            f32x4__ne = 0x41,
            f32x4__lt = 0x42,
            f32x4__gt = 0x43,
            f32x4__le = 0x44,
            f32x4__ge = 0x45,
            f64x2__eq = 0x46,
            f64x2__ne = 0x47,
            f64x2__lt = 0x48,
            f64x2__gt = 0x49,
            f64x2__le = 0x4a,
            f64x2__ge = 0x4b,
            v128__not = 0x4c,
            v128__and = 0x4d,
            v128__or = 0x4e,
            v128__xor = 0x4f,
            v128__bitselect = 0x50,
            i8x16__neg = 0x51,
            i8x16__any_true = 0x52,
            i8x16__all_true = 0x53,
            i8x16__shl = 0x54,
            i8x16__shr_s = 0x55,
            i8x16__shr_u = 0x56,
            i8x16__add = 0x57,
            i8x16__add_saturate_s = 0x58,
            i8x16__add_saturate_u = 0x59,
            i8x16__sub = 0x5a,
            i8x16__sub_saturate_s = 0x5b,
            i8x16__sub_saturate_u = 0x5c,
            i8x16__mul = 0x5d,
            i16x8__neg = 0x62,
            i16x8__any_true = 0x63,
            i16x8__all_true = 0x64,
            i16x8__shl = 0x65,
            i16x8__shr_s = 0x66,
            i16x8__shr_u = 0x67,
            i16x8__add = 0x68,
            i16x8__add_saturate_s = 0x69,
            i16x8__add_saturate_u = 0x6a,
            i16x8__sub = 0x6b,
            i16x8__sub_saturate_s = 0x6c,
            i16x8__sub_saturate_u = 0x6d,
            i16x8__mul = 0x6e,
            i32x4__neg = 0x73,
            i32x4__any_true = 0x74,
            i32x4__all_true = 0x75,
            i32x4__shl = 0x76,
            i32x4__shr_s = 0x77,
            i32x4__shr_u = 0x78,
            i32x4__add = 0x79,
            i32x4__sub = 0x7c,
            i32x4__mul = 0x7f,
            i64x2__neg = 0x84,
            i64x2__any_true = 0x85,
            i64x2__all_true = 0x86,
            i64x2__shl = 0x87,
            i64x2__shr_s = 0x88,
            i64x2__shr_u = 0x89,
            i64x2__add = 0x8a,
            i64x2__sub = 0x8d,
            f32x4__abs = 0x95,
            f32x4__neg = 0x96,
            f32x4__sqrt = 0x97,
            f32x4__add = 0x9a,
            f32x4__sub = 0x9b,
            f32x4__mul = 0x9c,
            f32x4__div = 0x9d,
            f32x4__min = 0x9e,
            f32x4__max = 0x9f,
            f64x2__abs = 0xa0,
            f64x2__neg = 0xa1,
            f64x2__sqrt = 0xa2,
            f64x2__add = 0xa5,
            f64x2__sub = 0xa6,
            f64x2__mul = 0xa7,
            f64x2__div = 0xa8,
            f64x2__min = 0xa9,
            f64x2__max = 0xaa,
            i32x4__trunc_sat_f32x4_s = 0xab,
            i32x4__trunc_sat_f32x4_u = 0xac,
            i64x2__trunc_sat_f64x2_s = 0xad,
            i64x2__trunc_sat_f64x2_u = 0xae,
            f32x4__convert_i32x4_s = 0xaf,
            f32x4__convert_i32x4_u = 0xb0,
            f64x2__convert_i64x2_s = 0xb1,
            f64x2__convert_i64x2_u = 0xb2,
            v8x16__swizzle = 0xc0,
            v8x16__shuffle = 0xc1,
            i8x16__load_splat = 0xc2,
            i16x8__load_splat = 0xc3,
            i32x4__load_splat = 0xc4,
            i64x2__load_splat = 0xc5,
            i8x16__narrow_i16x8_s = 0xc6,
            i8x16__narrow_i16x8_u = 0xc7,
            i16x8__narrow_i32x4_s = 0xc8,
            i16x8__narrow_i32x4_u = 0xc9,
            i16x8__widen_low_i8x16_s = 0xca,
            i16x8__widen_high_i8x16_s = 0xcb,
            i16x8__widen_low_i8x16_u = 0xcc,
            i16x8__widen_high_i8x16_u = 0xcd,
            i32x4__widen_low_i16x8_s = 0xce,
            i32x4__widen_high_i16x8_s = 0xcf,
            i32x4__widen_low_i16x8_u = 0xd0,
            i32x4__widen_high_i16x8_u = 0xd1,
            i16x8__load8x8_s = 0xd2,
            i16x8__load8x8_u = 0xd3,
            i32x4__load16x4_s = 0xd4,
            i32x4__load16x4_u = 0xd5,
            i64x2__load32x2_s = 0xd6,
            i64x2__load32x2_u = 0xd7,
            v128__andnot = 0xd8,
            i8x16__avgr_u = 0xd9,
            i16x8__avgr_u = 0xda,
            i8x16__abs = 0xe1,
            i16x8__abs = 0xe2,
            i32x4__abs = 0xe3,

            // THREAD
            atomic__notify = 0x00,
            i32__atomic__wait = 0x01,
            i64__atomic__wait = 0x02,
            i32__atomic__load = 0x10,
            i64__atomic__load = 0x11,
            i32__atomic__load8_u = 0x12,
            i32__atomic__load16_u = 0x13,
            i64__atomic__load8_u = 0x14,
            i64__atomic__load16_u = 0x15,
            i64__atomic__load32_u = 0x16,
            i32__atomic__store = 0x17,
            i64__atomic__store = 0x18,
            i32__atomic__store8 = 0x19,
            i32__atomic__store16 = 0x1a,
            i64__atomic__store8 = 0x1b,
            i64__atomic__store16 = 0x1c,
            i64__atomic__store32 = 0x1d,
            i32__atomic__rmw__add = 0x1e,
            i64__atomic__rmw__add = 0x1f,
            i32__atomic__rmw8__add_u = 0x20,
            i32__atomic__rmw16__add_u = 0x21,
            i64__atomic__rmw8__add_u = 0x22,
            i64__atomic__rmw16__add_u = 0x23,
            i64__atomic__rmw32__add_u = 0x24,
            i32__atomic__rmw__sub = 0x25,
            i64__atomic__rmw__sub = 0x26,
            i32__atomic__rmw8__sub_u = 0x27,
            i32__atomic__rmw16__sub_u = 0x28,
            i64__atomic__rmw8__sub_u = 0x29,
            i64__atomic__rmw16__sub_u = 0x2a,
            i64__atomic__rmw32__sub_u = 0x2b,
            i32__atomic__rmw__and = 0x2c,
            i64__atomic__rmw__and = 0x2d,
            i32__atomic__rmw8__and_u = 0x2e,
            i32__atomic__rmw16__and_u = 0x2f,
            i64__atomic__rmw8__and_u = 0x30,
            i64__atomic__rmw16__and_u = 0x31,
            i64__atomic__rmw32__and_u = 0x32,
            i32__atomic__rmw__or = 0x33,
            i64__atomic__rmw__or = 0x34,
            i32__atomic__rmw8__or_u = 0x35,
            i32__atomic__rmw16__or_u = 0x36,
            i64__atomic__rmw8__or_u = 0x37,
            i64__atomic__rmw16__or_u = 0x38,
            i64__atomic__rmw32__or_u = 0x39,
            i32__atomic__rmw__xor = 0x3a,
            i64__atomic__rmw__xor = 0x3b,
            i32__atomic__rmw8__xor_u = 0x3c,
            i32__atomic__rmw16__xor_u = 0x3d,
            i64__atomic__rmw8__xor_u = 0x3e,
            i64__atomic__rmw16__xor_u = 0x3f,
            i64__atomic__rmw32__xor_u = 0x40,
            i32__atomic__rmw__xchg = 0x41,
            i64__atomic__rmw__xchg = 0x42,
            i32__atomic__rmw8__xchg_u = 0x43,
            i32__atomic__rmw16__xchg_u = 0x44,
            i64__atomic__rmw8__xchg_u = 0x45,
            i64__atomic__rmw16__xchg_u = 0x46,
            i64__atomic__rmw32__xchg_u = 0x47,
            i32__atomic__rmw__cmpxchg = 0x48,
            i64__atomic__rmw__cmpxchg = 0x49,
            i32__atomic__rmw8__cmpxchg_u = 0x4a,
            i32__atomic__rmw16__cmpxchg_u = 0x4b,
            i64__atomic__rmw8__cmpxchg_u = 0x4c,
            i64__atomic__rmw16__cmpxchg_u = 0x4d,
            i64__atomic__rmw32__cmpxchg_u = 0x4e,
        };

        Opcode() = default;

        Opcode(uint8_t prefix, uint32_t code)
          : value((prefix << 24) | code)
        {
        }

        explicit Opcode(uint32_t v)
          : value(v)
        {
        }

        std::string_view getName() const;
        ParameterEncoding getParameterEncoding() const;
        uint32_t getAlign() const;
        bool isValid() const;
        
        bool operator==(uint32_t v) const
        {
            return value == v;
        }

        bool operator!=(uint32_t v) const
        {
            return value != v;
        }

        explicit operator uint32_t() const
        {
            return value;
        }

        OpcodePrefix getPrefix() const
        {
            return OpcodePrefix(uint8_t(value >> 24));
        }

        uint32_t getCode() const
        {
            return value & 0xffffff;
        }

        static std::optional<Opcode> fromString(std::string_view name);

    private:
        struct Info
        {
            uint32_t opcode;
            ParameterEncoding encoding = ParameterEncoding::none;
            SignatureCode signatureCode = SignatureCode::void_;
            std::string_view name;
            uint32_t align = 0;
        };

        struct Entry
        {
            Entry(std::string_view n, uint32_t i)
              : name(n), index(i)
            {
            }

            std::string_view name;
            uint32_t index;
        };

        Info* getInfo() const;
        static void buildMap();

        static Info info[];
        static std::vector<Entry> map;
        uint32_t value = 0;
};

inline bool operator==(uint32_t v, const Opcode& opcode)
{
    return opcode == v;
}

inline bool operator!=(uint32_t v, const Opcode& opcode)
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
            event,
            max = event
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

        explicit operator int32_t() const
        {
            return value;
        }

        static std::optional<ValueType> getEncoding(std::string_view v);

    private:
        int32_t value = 0;
        
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

class EventType
{
    public:
        enum : uint8_t
        {
            exception = 0
        };

        EventType() = default;

        explicit EventType(uint8_t v)
          : value(v)
        {
        }

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

        bool isValid() const
        {
            return value == exception;
        }

        std::string_view getName() const;

    private:
        uint8_t value = 0;
};

inline bool operator==(uint8_t v, const EventType& event)
{
    return event == v;
}

inline bool operator!=(uint8_t v, const EventType& event)
{
    return event != v;
}

inline std::ostream& operator<<(std::ostream& os, const EventType& value)
{
    return os << value.getName();
}

class ExternalType
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

        ExternalType() = default;
        ExternalType(uint8_t v)
          : value(v)
        {
        }

        explicit operator uint8_t() const
        {
            return value;
        }

        static std::optional<ExternalType> getEncoding(std::string_view name);

    private:
        uint8_t value = max + 1;
};

inline bool operator==(uint8_t v, const ExternalType& type)
{
    return type == v;
}

inline bool operator!=(uint8_t v, const ExternalType& type)
{
    return type != v;
}

inline std::ostream& operator<<(std::ostream& os, ExternalType value)
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
