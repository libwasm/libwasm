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

namespace libwasm
{
enum class ImmediateType
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
    elementIdx,
    eventIdx,
    functionIdx,
    globalIdx,
    labelIdx,
    localIdx,
    segmentIdx,
    segmentIdxMem,
    mem,
    memMem,
    tableElementIdx,
    table,
    tableTable,
    lane2Idx,
    lane4Idx,
    lane8Idx,
    lane16Idx,
    lane32Idx,
    shuffle,
    brTable,
    memory,
    memory0,
    indirect,
    depthEventIdx,
};

class OpcodePrefix
{
    public:
        enum Value : uint8_t
        {
            extns = 0xfc,
            simd = 0xfd,
            thread = 0xfe,
        };

        OpcodePrefix() = default;
        OpcodePrefix(int32_t v)
          : value(Value(v))
        {
        }

        OpcodePrefix(Value v)
          : value(v)
        {
        }

        operator Value() const
        {
            return value;
        }


        bool isValid() const
        {
            return value == simd || value == thread || value == extns;
        }

    private:
        Value value = Value(0);
};

enum class SignatureCode
{
    void_,
    f32_,
    f32__f32,
    f32__f32_f32,
    f32__f64,
    f32__i32,
    f32__i64,
    f32__v128,
    f64_,
    f64__f32,
    f64__f64,
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
    void__i32,
    void__i32_f32,
    void__i32_f64,
    void__i32_i32,
    void__i32_i32_i32,
    void__i32_i64,
    void__i32_v128,
    special,
};

class Opcode
{
    static const uint32_t extns = OpcodePrefix::extns << 24;
    static const uint32_t simd = OpcodePrefix::simd << 24;
    static const uint32_t thread = OpcodePrefix::thread << 24;

    public:
        struct Info
        {
            uint32_t opcode;
            ImmediateType type = ImmediateType::none;
            SignatureCode signatureCode = SignatureCode::void_;
            std::string_view name;
            uint32_t align = 0;
        };

        enum Value : uint32_t
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
            table__get = 0x25,
            table__set = 0x26,
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
            ref__null = 0xd0,
            ref__is_null = 0xd1,
            ref__func = 0xd2,
            alloca = 0xe0,
            br_unless = 0xe1,
            call_host = 0xe2,
            data = 0xe3,
            drop_keep = 0xe4,
            i32__trunc_sat_f32_s = extns | 0x00,
            i32__trunc_sat_f32_u = extns | 0x01,
            i32__trunc_sat_f64_s = extns | 0x02,
            i32__trunc_sat_f64_u = extns | 0x03,
            i64__trunc_sat_f32_s = extns | 0x04,
            i64__trunc_sat_f32_u = extns | 0x05,
            i64__trunc_sat_f64_s = extns | 0x06,
            i64__trunc_sat_f64_u = extns | 0x07,
            memory__init = extns | 0x08,
            data__drop = extns | 0x09,
            memory__copy = extns | 0x0a,
            memory__fill = extns | 0x0b,
            table__init = extns | 0x0c,
            elem__drop = extns | 0x0d,
            table__copy = extns | 0x0e,
            table__grow = extns | 0x0f,
            table__size = extns | 0x10,
            table__fill = extns | 0x11,

            // SIMD
            v128__load = simd | 0x00,
            v128__store = simd | 0x01,
            v128__const = simd | 0x02,
            v8x16__shuffle = simd | 0x03,
            i8x16__splat = simd | 0x04,
            i8x16__extract_lane_s = simd | 0x05,
            i8x16__extract_lane_u = simd | 0x06,
            i8x16__replace_lane = simd | 0x07,
            i16x8__splat = simd | 0x08,
            i16x8__extract_lane_s = simd | 0x09,
            i16x8__extract_lane_u = simd | 0x0a,
            i16x8__replace_lane = simd | 0x0b,
            i32x4__splat = simd | 0x0c,
            i32x4__extract_lane = simd | 0x0d,
            i32x4__replace_lane = simd | 0x0e,
            i64x2__splat = simd | 0x0f,
            i64x2__extract_lane = simd | 0x10,
            i64x2__replace_lane = simd | 0x11,
            f32x4__splat = simd | 0x12,
            f32x4__extract_lane = simd | 0x13,
            f32x4__replace_lane = simd | 0x14,
            f64x2__splat = simd | 0x15,
            f64x2__extract_lane = simd | 0x16,
            f64x2__replace_lane = simd | 0x17,
            i8x16__eq = simd | 0x18,
            i8x16__ne = simd | 0x19,
            i8x16__lt_s = simd | 0x1a,
            i8x16__lt_u = simd | 0x1b,
            i8x16__gt_s = simd | 0x1c,
            i8x16__gt_u = simd | 0x1d,
            i8x16__le_s = simd | 0x1e,
            i8x16__le_u = simd | 0x1f,
            i8x16__ge_s = simd | 0x20,
            i8x16__ge_u = simd | 0x21,
            i16x8__eq = simd | 0x22,
            i16x8__ne = simd | 0x23,
            i16x8__lt_s = simd | 0x24,
            i16x8__lt_u = simd | 0x25,
            i16x8__gt_s = simd | 0x26,
            i16x8__gt_u = simd | 0x27,
            i16x8__le_s = simd | 0x28,
            i16x8__le_u = simd | 0x29,
            i16x8__ge_s = simd | 0x2a,
            i16x8__ge_u = simd | 0x2b,
            i32x4__eq = simd | 0x2c,
            i32x4__ne = simd | 0x2d,
            i32x4__lt_s = simd | 0x2e,
            i32x4__lt_u = simd | 0x2f,
            i32x4__gt_s = simd | 0x30,
            i32x4__gt_u = simd | 0x31,
            i32x4__le_s = simd | 0x32,
            i32x4__le_u = simd | 0x33,
            i32x4__ge_s = simd | 0x34,
            i32x4__ge_u = simd | 0x35,
            f32x4__eq = simd | 0x40,
            f32x4__ne = simd | 0x41,
            f32x4__lt = simd | 0x42,
            f32x4__gt = simd | 0x43,
            f32x4__le = simd | 0x44,
            f32x4__ge = simd | 0x45,
            f64x2__eq = simd | 0x46,
            f64x2__ne = simd | 0x47,
            f64x2__lt = simd | 0x48,
            f64x2__gt = simd | 0x49,
            f64x2__le = simd | 0x4a,
            f64x2__ge = simd | 0x4b,
            v128__not = simd | 0x4c,
            v128__and = simd | 0x4d,
            v128__or = simd | 0x4e,
            v128__xor = simd | 0x4f,
            v128__bitselect = simd | 0x50,
            i8x16__neg = simd | 0x51,
            i8x16__any_true = simd | 0x52,
            i8x16__all_true = simd | 0x53,
            i8x16__shl = simd | 0x54,
            i8x16__shr_s = simd | 0x55,
            i8x16__shr_u = simd | 0x56,
            i8x16__add = simd | 0x57,
            i8x16__add_saturate_s = simd | 0x58,
            i8x16__add_saturate_u = simd | 0x59,
            i8x16__sub = simd | 0x5a,
            i8x16__sub_saturate_s = simd | 0x5b,
            i8x16__sub_saturate_u = simd | 0x5c,
            i8x16__mul = simd | 0x5d,
            i8x16__min_s = simd | 0x5e,
            i8x16__min_u = simd | 0x5f,
            i8x16__max_s = simd | 0x60,
            i8x16__max_u = simd | 0x61,
            i16x8__neg = simd | 0x62,
            i16x8__any_true = simd | 0x63,
            i16x8__all_true = simd | 0x64,
            i16x8__shl = simd | 0x65,
            i16x8__shr_s = simd | 0x66,
            i16x8__shr_u = simd | 0x67,
            i16x8__add = simd | 0x68,
            i16x8__add_saturate_s = simd | 0x69,
            i16x8__add_saturate_u = simd | 0x6a,
            i16x8__sub = simd | 0x6b,
            i16x8__sub_saturate_s = simd | 0x6c,
            i16x8__sub_saturate_u = simd | 0x6d,
            i16x8__mul = simd | 0x6e,
            i16x8__min_s = simd | 0x6f,
            i16x8__min_u = simd | 0x70,
            i16x8__max_s = simd | 0x71,
            i16x8__max_u = simd | 0x72,
            i32x4__neg = simd | 0x73,
            i32x4__any_true = simd | 0x74,
            i32x4__all_true = simd | 0x75,
            i32x4__shl = simd | 0x76,
            i32x4__shr_s = simd | 0x77,
            i32x4__shr_u = simd | 0x78,
            i32x4__add = simd | 0x79,
            i32x4__sub = simd | 0x7c,
            i32x4__mul = simd | 0x7f,
            i32x4__min_s = simd | 0x80,
            i32x4__min_u = simd | 0x81,
            i32x4__max_s = simd | 0x82,
            i32x4__max_u = simd | 0x83,
            i64x2__neg = simd | 0x84,
            i64x2__any_true = simd | 0x85,
            i64x2__all_true = simd | 0x86,
            i64x2__shl = simd | 0x87,
            i64x2__shr_s = simd | 0x88,
            i64x2__shr_u = simd | 0x89,
            i64x2__add = simd | 0x8a,
            i64x2__sub = simd | 0x8d,
            i64x2__mul = simd | 0x90,
            f32x4__abs = simd | 0x95,
            f32x4__neg = simd | 0x96,
            f32x4__sqrt = simd | 0x97,
            f32x4__add = simd | 0x9a,
            f32x4__sub = simd | 0x9b,
            f32x4__mul = simd | 0x9c,
            f32x4__div = simd | 0x9d,
            f32x4__min = simd | 0x9e,
            f32x4__max = simd | 0x9f,
            f64x2__abs = simd | 0xa0,
            f64x2__neg = simd | 0xa1,
            f64x2__sqrt = simd | 0xa2,
            f64x2__add = simd | 0xa5,
            f64x2__sub = simd | 0xa6,
            f64x2__mul = simd | 0xa7,
            f64x2__div = simd | 0xa8,
            f64x2__min = simd | 0xa9,
            f64x2__max = simd | 0xaa,
            i32x4__trunc_sat_f32x4_s = simd | 0xab,
            i32x4__trunc_sat_f32x4_u = simd | 0xac,
            i64x2__trunc_sat_f64x2_s = simd | 0xad,
            i64x2__trunc_sat_f64x2_u = simd | 0xae,
            f32x4__convert_i32x4_s = simd | 0xaf,
            f32x4__convert_i32x4_u = simd | 0xb0,
            f64x2__convert_i64x2_s = simd | 0xb1,
            f64x2__convert_i64x2_u = simd | 0xb2,
            v8x16__swizzle = simd | 0xc0,
            v8x16__load_splat = simd | 0xc2,
            v16x8__load_splat = simd | 0xc3,
            v32x4__load_splat = simd | 0xc4,
            v64x2__load_splat = simd | 0xc5,
            i8x16__narrow_i16x8_s = simd | 0xc6,
            i8x16__narrow_i16x8_u = simd | 0xc7,
            i16x8__narrow_i32x4_s = simd | 0xc8,
            i16x8__narrow_i32x4_u = simd | 0xc9,
            i16x8__widen_low_i8x16_s = simd | 0xca,
            i16x8__widen_high_i8x16_s = simd | 0xcb,
            i16x8__widen_low_i8x16_u = simd | 0xcc,
            i16x8__widen_high_i8x16_u = simd | 0xcd,
            i32x4__widen_low_i16x8_s = simd | 0xce,
            i32x4__widen_high_i16x8_s = simd | 0xcf,
            i32x4__widen_low_i16x8_u = simd | 0xd0,
            i32x4__widen_high_i16x8_u = simd | 0xd1,
            i16x8__load8x8_s = simd | 0xd2,
            i16x8__load8x8_u = simd | 0xd3,
            i32x4__load16x4_s = simd | 0xd4,
            i32x4__load16x4_u = simd | 0xd5,
            i64x2__load32x2_s = simd | 0xd6,
            i64x2__load32x2_u = simd | 0xd7,
            v128__andnot = simd | 0xd8,
            i8x16__avgr_u = simd | 0xd9,
            i16x8__avgr_u = simd | 0xda,
            i8x16__abs = simd | 0xe1,
            i16x8__abs = simd | 0xe2,
            i32x4__abs = simd | 0xe3,

            // THREAD
            atomic__notify = thread | 0x00,
            i32__atomic__wait = thread | 0x01,
            i64__atomic__wait = thread | 0x02,
            i32__atomic__load = thread | 0x10,
            i64__atomic__load = thread | 0x11,
            i32__atomic__load8_u = thread | 0x12,
            i32__atomic__load16_u = thread | 0x13,
            i64__atomic__load8_u = thread | 0x14,
            i64__atomic__load16_u = thread | 0x15,
            i64__atomic__load32_u = thread | 0x16,
            i32__atomic__store = thread | 0x17,
            i64__atomic__store = thread | 0x18,
            i32__atomic__store8 = thread | 0x19,
            i32__atomic__store16 = thread | 0x1a,
            i64__atomic__store8 = thread | 0x1b,
            i64__atomic__store16 = thread | 0x1c,
            i64__atomic__store32 = thread | 0x1d,
            i32__atomic__rmw__add = thread | 0x1e,
            i64__atomic__rmw__add = thread | 0x1f,
            i32__atomic__rmw8__add_u = thread | 0x20,
            i32__atomic__rmw16__add_u = thread | 0x21,
            i64__atomic__rmw8__add_u = thread | 0x22,
            i64__atomic__rmw16__add_u = thread | 0x23,
            i64__atomic__rmw32__add_u = thread | 0x24,
            i32__atomic__rmw__sub = thread | 0x25,
            i64__atomic__rmw__sub = thread | 0x26,
            i32__atomic__rmw8__sub_u = thread | 0x27,
            i32__atomic__rmw16__sub_u = thread | 0x28,
            i64__atomic__rmw8__sub_u = thread | 0x29,
            i64__atomic__rmw16__sub_u = thread | 0x2a,
            i64__atomic__rmw32__sub_u = thread | 0x2b,
            i32__atomic__rmw__and = thread | 0x2c,
            i64__atomic__rmw__and = thread | 0x2d,
            i32__atomic__rmw8__and_u = thread | 0x2e,
            i32__atomic__rmw16__and_u = thread | 0x2f,
            i64__atomic__rmw8__and_u = thread | 0x30,
            i64__atomic__rmw16__and_u = thread | 0x31,
            i64__atomic__rmw32__and_u = thread | 0x32,
            i32__atomic__rmw__or = thread | 0x33,
            i64__atomic__rmw__or = thread | 0x34,
            i32__atomic__rmw8__or_u = thread | 0x35,
            i32__atomic__rmw16__or_u = thread | 0x36,
            i64__atomic__rmw8__or_u = thread | 0x37,
            i64__atomic__rmw16__or_u = thread | 0x38,
            i64__atomic__rmw32__or_u = thread | 0x39,
            i32__atomic__rmw__xor = thread | 0x3a,
            i64__atomic__rmw__xor = thread | 0x3b,
            i32__atomic__rmw8__xor_u = thread | 0x3c,
            i32__atomic__rmw16__xor_u = thread | 0x3d,
            i64__atomic__rmw8__xor_u = thread | 0x3e,
            i64__atomic__rmw16__xor_u = thread | 0x3f,
            i64__atomic__rmw32__xor_u = thread | 0x40,
            i32__atomic__rmw__xchg = thread | 0x41,
            i64__atomic__rmw__xchg = thread | 0x42,
            i32__atomic__rmw8__xchg_u = thread | 0x43,
            i32__atomic__rmw16__xchg_u = thread | 0x44,
            i64__atomic__rmw8__xchg_u = thread | 0x45,
            i64__atomic__rmw16__xchg_u = thread | 0x46,
            i64__atomic__rmw32__xchg_u = thread | 0x47,
            i32__atomic__rmw__cmpxchg = thread | 0x48,
            i64__atomic__rmw__cmpxchg = thread | 0x49,
            i32__atomic__rmw8__cmpxchg_u = thread | 0x4a,
            i32__atomic__rmw16__cmpxchg_u = thread | 0x4b,
            i64__atomic__rmw8__cmpxchg_u = thread | 0x4c,
            i64__atomic__rmw16__cmpxchg_u = thread | 0x4d,
            i64__atomic__rmw32__cmpxchg_u = thread | 0x4e,
        };

        Opcode() = default;
        Opcode(int32_t v)
          : value(Value(v))
        {
        }

        Opcode(Value v)
          : value(v)
        {
        }

        operator Value() const
        {
            return value;
        }


        Opcode(uint8_t prefix, uint32_t code)
          : value(Value((prefix << 24) | code))
        {
        }

        std::string_view getName() const;
        ImmediateType getImmediateType() const;
        uint32_t getAlign() const;
        bool isValid() const;
        
        OpcodePrefix getPrefix() const
        {
            return OpcodePrefix(uint8_t(value >> 24));
        }

        uint32_t getCode() const
        {
            return value & 0xffffff;
        }

        const Info* getInfo() const;

        static std::optional<Opcode> fromString(std::string_view name);

    private:
        struct Entry
        {
            Entry(std::string_view n, uint32_t i)
              : name(n), index(i)
            {
            }

            std::string_view name;
            uint32_t index;
        };

        static void buildMap();

        Value value = Value(0);

        static Info info[];
        static std::vector<Entry> map;

        class Initializer
        {
            public:
                Initializer()
                {
                    Opcode::buildMap();
                }
        };

        friend class Initializer;

        static Initializer initializer;
};

inline std::ostream& operator<<(std::ostream& os, Opcode opcode)
{
    return os << opcode.getName();
}

class SectionType
{
    public:
        enum Value : uint8_t
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

        SectionType() = default;
        SectionType(int32_t v)
          : value(Value(v))
        {
        }

        SectionType(Value v)
          : value(v)
        {
        }

        operator Value() const
        {
            return value;
        }

        bool isValid() const;
        std::string_view getName() const;
        
    private:
        Value value = Value(max + 1);
};

inline std::ostream& operator<<(std::ostream& os, SectionType type)
{
    return os << type.getName();
}

class ValueType
{
    public:
        enum Value : int32_t
        {
            i32 = -0x01,      // 0x7f
            i64 = -0x02,      // 0x7e
            f32 = -0x03,      // 0x7d
            f64 = -0x04,      // 0x7c
            v128 = -0x05,     // 0x7b
            funcref = -0x10,  // 0x70
            anyref = -0x11,   // 0x6f
            nullref = -0x12,  // 0c6e
            exnref = -0x18,   // 0x68
            void_ = -0x40,    // 0x40
        };

        ValueType() = default;
        ValueType(int32_t v)
          : value(Value(v))
        {
        }

        ValueType(Value v)
          : value(v)
        {
        }

        operator Value() const
        {
            return value;
        }

        bool isValid() const;
        bool isValidNumeric() const;
        bool isValidRef() const;
        std::string_view getName() const;
        
        static std::optional<ValueType> getEncoding(std::string_view v);

    private:
        Value value = Value(0);
        
};

inline std::ostream& operator<<(std::ostream& os, ValueType type)
{
    return os << type.getName();
}

class EventType
{
    public:
        enum Value : uint8_t
        {
            exception = 0
        };

        EventType() = default;
        EventType(int32_t v)
          : value(Value(v))
        {
        }

        EventType(Value v)
          : value(v)
        {
        }

        operator Value() const
        {
            return value;
        }

        bool isValid() const
        {
            return value == exception;
        }

        std::string_view getName() const;

    private:
        Value value = Value(0);
};

inline std::ostream& operator<<(std::ostream& os, const EventType& value)
{
    return os << value.getName();
}

class ExternalType
{
    public:
        enum Value : uint8_t
        {
            function,
            table,
            memory,
            global,
            event,
            max = event
        };

        ExternalType() = default;
        ExternalType(int32_t v)
          : value(Value(v))
        {
        }

        ExternalType(Value v)
          : value(v)
        {
        }

        operator Value() const
        {
            return value;
        }

        bool isValid() const;
        std::string_view getName() const;
        
        static std::optional<ExternalType> getEncoding(std::string_view name);

    private:
        Value value = Value(max + 1);
};

inline std::ostream& operator<<(std::ostream& os, ExternalType value)
{
    return os << value.getName();
}

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

enum SymbolFlags : uint32_t
{
    SymbolFlagNone = 0,
    SymbolFlagWeak = 0x1,
    SymbolFlagLocal = 0x2,
    SymbolFlagHidden = 0x4,
    SymbolFlagUndefined = 0x10,
    SymbolFlagExported = 0x20,
    SymbolFlagExplicitName = 0x40,
    SymbolFlagNoStrip = 0x80
};

enum SegmentFlags : uint8_t {
  SegmentFlagNone = 0,
  SegmentFlagPassive = 1,
  SegmentFlagExplicitIndex = 2,
  SegmentFlagDEclared = SegmentFlags(SegmentFlagPassive | SegmentFlagExplicitIndex),
  SegmentFlagElemExpr = 4,

  SegmentFlagMax = (SegmentFlagElemExpr << 1) - 1,
};

struct Limits
{
    enum : uint8_t
    {
        none = 0,
        hasMaxFlag = 0x1,
        isSharedFlag = 0x2
    };

    Limits() = default;

    Limits(uint32_t min)
      : min(min)
    {
    }

    Limits(uint32_t min, uint32_t max)
      : flags(hasMaxFlag), min(min), max(max)
    {
    }

    bool hasMax() const
    {
        return (flags & hasMaxFlag) != 0;
    }

    bool isShared() const
    {
        return (flags & isSharedFlag) != 0;
    }

    uint8_t flags = none;
    uint32_t min = 0;
    uint32_t max = 0;

    void show(std::ostream& os);
    void generate(std::ostream& os);
};
};

#endif
