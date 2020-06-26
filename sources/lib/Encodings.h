// Encodings.h

#ifndef OPCODES_H
#define OPCODES_H

#include "common.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace libwasm
{
enum class ImmediateType
{
    none,
    valueType,
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
    refType,
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
    void_,
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

        struct NameEntry
        {
            NameEntry(std::string_view n, uint32_t o)
              : name(n), opcode(o)
            {
            }

            std::string_view name;
            uint32_t opcode = 0;
        };

        struct OpcodeEntry
        {
            OpcodeEntry(uint32_t o, uint32_t i)
              : opcode(o), infoIndex(i)
            {
            }

            uint32_t opcode = 0;
            uint32_t infoIndex = 0;
        };

        struct Index
        {
            Index(unsigned i, unsigned s)
              : index(i), size(s)
            {
            }

            unsigned index = 0;
            unsigned size = 0;
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

            //EXTNS
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
            i16x8__load8x8_s = simd | 0x01,
            i16x8__load8x8_u = simd | 0x02,
            i32x4__load16x4_s = simd | 0x03,
            i32x4__load16x4_u = simd | 0x04,
            i64x2__load32x2_s = simd | 0x05,
            i64x2__load32x2_u = simd | 0x06,
            v8x16__load_splat = simd | 0x07,
            v16x8__load_splat = simd | 0x08,
            v32x4__load_splat = simd | 0x09,
            v64x2__load_splat = simd | 0x0a,
            v128__store = simd | 0x0b,
            v128__const = simd | 0x0c,
            v8x16__shuffle = simd | 0x0d,
            v8x16__swizzle = simd | 0x0e,
            i8x16__splat = simd | 0x0f,
            i16x8__splat = simd | 0x10,
            i32x4__splat = simd | 0x11,
            i64x2__splat = simd | 0x12,
            f32x4__splat = simd | 0x13,
            f64x2__splat = simd | 0x14,
            i8x16__extract_lane_s = simd | 0x15,
            i8x16__extract_lane_u = simd | 0x16,
            i8x16__replace_lane = simd | 0x17,
            i16x8__extract_lane_s = simd | 0x18,
            i16x8__extract_lane_u = simd | 0x19,
            i16x8__replace_lane = simd | 0x1a,
            i32x4__extract_lane = simd | 0x1b,
            i32x4__replace_lane = simd | 0x1c,
            i64x2__extract_lane = simd | 0x1d,
            i64x2__replace_lane = simd | 0x1e,
            f32x4__extract_lane = simd | 0x1f,
            f32x4__replace_lane = simd | 0x20,
            f64x2__extract_lane = simd | 0x21,
            f64x2__replace_lane = simd | 0x22,
            i8x16__eq = simd | 0x23,
            i8x16__ne = simd | 0x24,
            i8x16__lt_s = simd | 0x25,
            i8x16__lt_u = simd | 0x26,
            i8x16__gt_s = simd | 0x27,
            i8x16__gt_u = simd | 0x28,
            i8x16__le_s = simd | 0x29,
            i8x16__le_u = simd | 0x2a,
            i8x16__ge_s = simd | 0x2b,
            i8x16__ge_u = simd | 0x2c,
            i16x8__eq = simd | 0x2d,
            i16x8__ne = simd | 0x2e,
            i16x8__lt_s = simd | 0x2f,
            i16x8__lt_u = simd | 0x30,
            i16x8__gt_s = simd | 0x31,
            i16x8__gt_u = simd | 0x32,
            i16x8__le_s = simd | 0x33,
            i16x8__le_u = simd | 0x34,
            i16x8__ge_s = simd | 0x35,
            i16x8__ge_u = simd | 0x36,
            i32x4__eq = simd | 0x37,
            i32x4__ne = simd | 0x38,
            i32x4__lt_s = simd | 0x39,
            i32x4__lt_u = simd | 0x3a,
            i32x4__gt_s = simd | 0x3b,
            i32x4__gt_u = simd | 0x3c,
            i32x4__le_s = simd | 0x3d,
            i32x4__le_u = simd | 0x3e,
            i32x4__ge_s = simd | 0x3f,
            i32x4__ge_u = simd | 0x40,
            f32x4__eq = simd | 0x41,
            f32x4__ne = simd | 0x42,
            f32x4__lt = simd | 0x43,
            f32x4__gt = simd | 0x44,
            f32x4__le = simd | 0x45,
            f32x4__ge = simd | 0x46,
            f64x2__eq = simd | 0x47,
            f64x2__ne = simd | 0x48,
            f64x2__lt = simd | 0x49,
            f64x2__gt = simd | 0x4a,
            f64x2__le = simd | 0x4b,
            f64x2__ge = simd | 0x4c,
            v128__not = simd | 0x4d,
            v128__and = simd | 0x4e,
            v128__andnot = simd | 0x4f,
            v128__or = simd | 0x50,
            v128__xor = simd | 0x51,
            v128__bitselect = simd | 0x52,
            i8x16__abs = simd | 0x60,
            i8x16__neg = simd | 0x61,
            i8x16__any_true = simd | 0x62,
            i8x16__all_true = simd | 0x63,
            i8x16__narrow_i16x8_s = simd | 0x65,
            i8x16__narrow_i16x8_u = simd | 0x66,
            i8x16__shl = simd | 0x6b,
            i8x16__shr_s = simd | 0x6c,
            i8x16__shr_u = simd | 0x6d,
            i8x16__add = simd | 0x6e,
            i8x16__add_saturate_s = simd | 0x6f,
            i8x16__add_saturate_u = simd | 0x70,
            i8x16__sub = simd | 0x71,
            i8x16__sub_saturate_s = simd | 0x72,
            i8x16__sub_saturate_u = simd | 0x73,
            i8x16__min_s = simd | 0x76,
            i8x16__min_u = simd | 0x77,
            i8x16__max_s = simd | 0x78,
            i8x16__max_u = simd | 0x79,
            i8x16__avgr_u = simd | 0x7b,
            i16x8__abs = simd | 0x80,
            i16x8__neg = simd | 0x81,
            i16x8__any_true = simd | 0x82,
            i16x8__all_true = simd | 0x83,
            i16x8__narrow_i32x4_s = simd | 0x85,
            i16x8__narrow_i32x4_u = simd | 0x86,
            i16x8__widen_low_i8x16_s = simd | 0x87,
            i16x8__widen_high_i8x16_s = simd | 0x88,
            i16x8__widen_low_i8x16_u = simd | 0x89,
            i16x8__widen_high_i8x16_u = simd | 0x8a,
            i16x8__shl = simd | 0x8b,
            i16x8__shr_s = simd | 0x8c,
            i16x8__shr_u = simd | 0x8d,
            i16x8__add = simd | 0x8e,
            i16x8__add_saturate_s = simd | 0x8f,
            i16x8__add_saturate_u = simd | 0x90,
            i16x8__sub = simd | 0x91,
            i16x8__sub_saturate_s = simd | 0x92,
            i16x8__sub_saturate_u = simd | 0x93,
            i16x8__mul = simd | 0x95,
            i16x8__min_s = simd | 0x96,
            i16x8__min_u = simd | 0x97,
            i16x8__max_s = simd | 0x98,
            i16x8__max_u = simd | 0x99,
            i16x8__avgr_u = simd | 0x9b,
            i32x4__abs = simd | 0xa0,
            i32x4__neg = simd | 0xa1,
            i32x4__any_true = simd | 0xa2,
            i32x4__all_true = simd | 0xa3,
            i32x4__widen_low_i16x8_s = simd | 0xa7,
            i32x4__widen_high_i16x8_s = simd | 0xa8,
            i32x4__widen_low_i16x8_u = simd | 0xa9,
            i32x4__widen_high_i16x8_u = simd | 0xaa,
            i32x4__shl = simd | 0xab,
            i32x4__shr_s = simd | 0xac,
            i32x4__shr_u = simd | 0xad,
            i32x4__add = simd | 0xae,
            i32x4__sub = simd | 0xb1,
            i32x4__mul = simd | 0xb5,
            i32x4__min_s = simd | 0xb6,
            i32x4__min_u = simd | 0xb7,
            i32x4__max_s = simd | 0xb8,
            i32x4__max_u = simd | 0xb9,
            i64x2__neg = simd | 0xc1,
            i64x2__shl = simd | 0xcb,
            i64x2__shr_s = simd | 0xcc,
            i64x2__shr_u = simd | 0xcd,
            i64x2__add = simd | 0xce,
            i64x2__sub = simd | 0xd1,
            i64x2__mul = simd | 0xd5,

            f32x4__abs = simd | 0xe0,
            f32x4__neg = simd | 0xe1,
            f32x4__sqrt = simd | 0xe3,
            f32x4__add = simd | 0xe4,
            f32x4__sub = simd | 0xe5,
            f32x4__mul = simd | 0xe6,
            f32x4__div = simd | 0xe7,
            f32x4__min = simd | 0xe8,
            f32x4__max = simd | 0xe9,
            f64x2__abs = simd | 0xec,
            f64x2__neg = simd | 0xed,
            f64x2__sqrt = simd | 0xef,
            f64x2__add = simd | 0xf0,
            f64x2__sub = simd | 0xf1,
            f64x2__mul = simd | 0xf2,
            f64x2__div = simd | 0xf3,
            f64x2__min = simd | 0xf4,
            f64x2__max = simd | 0xf5,
            i32x4__trunc_sat_f32x4_s = simd | 0xf8,
            i32x4__trunc_sat_f32x4_u = simd | 0xf9,
            f32x4__convert_i32x4_s = simd | 0xfa,
            f32x4__convert_i32x4_u = simd | 0xfb,

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
            max = i64__atomic__rmw32__cmpxchg_u
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
        static const auto* getInfoTable()
        {
            return info;
        }

        unsigned hash() const
        {
            return unsigned(value) ^ ((unsigned(value) >> 18));
        }

    private:
        Value value = Value(0);

        static const Info info[];
        static const Index nameIndexes[512];
        static const NameEntry nameEntries[];
        static const Index opcodeIndexes[512];
        static const OpcodeEntry opcodeEntries[];
};

inline const Opcode::Info Opcode::info[] =
{
    { Opcode::unreachable, ImmediateType::none, SignatureCode::special, "unreachable", 0 },
    { Opcode::nop, ImmediateType::none, SignatureCode::void_, "nop", 0 },
    { Opcode::block, ImmediateType::block, SignatureCode::special, "block", 0 },
    { Opcode::loop, ImmediateType::block, SignatureCode::special, "loop", 0 },
    { Opcode::if_, ImmediateType::block, SignatureCode::special, "if", 0 },
    { Opcode::else_, ImmediateType::none, SignatureCode::special, "else", 0 },
    { Opcode::try_, ImmediateType::block, SignatureCode::special, "try", 0 },
    { Opcode::catch_, ImmediateType::none, SignatureCode::special, "catch", 0 },
    { Opcode::throw_, ImmediateType::eventIdx, SignatureCode::special, "throw", 0 },
    { Opcode::rethrow_, ImmediateType::none, SignatureCode::special, "rethrow", 0 },
    { Opcode::br_on_exn, ImmediateType::depthEventIdx, SignatureCode::void_, "br_on_exn", 0 },
    { Opcode::end, ImmediateType::none, SignatureCode::special, "end", 0 },
    { Opcode::br, ImmediateType::labelIdx, SignatureCode::special, "br", 0 },
    { Opcode::br_if, ImmediateType::labelIdx, SignatureCode::special, "br_if", 0 },
    { Opcode::br_table, ImmediateType::brTable, SignatureCode::special, "br_table", 0 },
    { Opcode::return_, ImmediateType::none, SignatureCode::special, "return", 0 },
    { Opcode::call, ImmediateType::functionIdx, SignatureCode::special, "call", 0 },
    { Opcode::call_indirect, ImmediateType::indirect, SignatureCode::special, "call_indirect", 0 },
    { Opcode::return_call, ImmediateType::functionIdx, SignatureCode::special, "return_call", 0 },
    { Opcode::return_call_indirect, ImmediateType::indirect, SignatureCode::special, "return_call_indirect", 0 },
    { Opcode::drop, ImmediateType::none, SignatureCode::special, "drop", 0 },
    { Opcode::select, ImmediateType::valueType, SignatureCode::special, "select", 0 },
    { Opcode::local__get, ImmediateType::localIdx, SignatureCode::special, "local.get", 0 },
    { Opcode::local__set, ImmediateType::localIdx, SignatureCode::special, "local.set", 0 },
    { Opcode::local__tee, ImmediateType::localIdx, SignatureCode::special, "local.tee", 0 },
    { Opcode::global__get, ImmediateType::globalIdx, SignatureCode::special, "global.get", 0 },
    { Opcode::global__set, ImmediateType::globalIdx, SignatureCode::special, "global.set", 0 },
    { Opcode::table__get, ImmediateType::table, SignatureCode::special, "table.get", 0 },
    { Opcode::table__set, ImmediateType::table, SignatureCode::special, "table.set", 0 },
    { Opcode::i32__load, ImmediateType::memory, SignatureCode::i32__i32, "i32.load", 4 },
    { Opcode::i64__load, ImmediateType::memory, SignatureCode::i64__i32, "i64.load", 8 },
    { Opcode::f32__load, ImmediateType::memory, SignatureCode::f32__i32, "f32.load", 4 },
    { Opcode::f64__load, ImmediateType::memory, SignatureCode::f64__i32, "f64.load", 8 },
    { Opcode::i32__load8_s, ImmediateType::memory, SignatureCode::i32__i32, "i32.load8_s", 1 },
    { Opcode::i32__load8_u, ImmediateType::memory, SignatureCode::i32__i32, "i32.load8_u", 1 },
    { Opcode::i32__load16_s, ImmediateType::memory, SignatureCode::i32__i32, "i32.load16_s", 2 },
    { Opcode::i32__load16_u, ImmediateType::memory, SignatureCode::i32__i32, "i32.load16_u", 2 },
    { Opcode::i64__load8_s, ImmediateType::memory, SignatureCode::i64__i32, "i64.load8_s", 1 },
    { Opcode::i64__load8_u, ImmediateType::memory, SignatureCode::i64__i32, "i64.load8_u", 1 },
    { Opcode::i64__load16_s, ImmediateType::memory, SignatureCode::i64__i32, "i64.load16_s", 2 },
    { Opcode::i64__load16_u, ImmediateType::memory, SignatureCode::i64__i32, "i64.load16_u", 2 },
    { Opcode::i64__load32_s, ImmediateType::memory, SignatureCode::i64__i32, "i64.load32_s", 4 },
    { Opcode::i64__load32_u, ImmediateType::memory, SignatureCode::i64__i32, "i64.load32_u", 4 },
    { Opcode::i32__store, ImmediateType::memory, SignatureCode::void__i32_i32, "i32.store", 4 },
    { Opcode::i64__store, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.store", 8 },
    { Opcode::f32__store, ImmediateType::memory, SignatureCode::void__i32_f32, "f32.store", 4 },
    { Opcode::f64__store, ImmediateType::memory, SignatureCode::void__i32_f64, "f64.store", 8 },
    { Opcode::i32__store8, ImmediateType::memory, SignatureCode::void__i32_i32, "i32.store8", 1 },
    { Opcode::i32__store16, ImmediateType::memory, SignatureCode::void__i32_i32, "i32.store16", 2 },
    { Opcode::i64__store8, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.store8", 1 },
    { Opcode::i64__store16, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.store16", 2 },
    { Opcode::i64__store32, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.store32", 4 },
    { Opcode::memory__size, ImmediateType::memory0, SignatureCode::i32_, "memory.size", 0 },
    { Opcode::memory__grow, ImmediateType::memory0, SignatureCode::i32__i32, "memory.grow", 0 },
    { Opcode::i32__const, ImmediateType::i32, SignatureCode::i32_, "i32.const", 0 },
    { Opcode::i64__const, ImmediateType::i64, SignatureCode::i64_, "i64.const", 0 },
    { Opcode::f32__const, ImmediateType::f32, SignatureCode::f32_, "f32.const", 0 },
    { Opcode::f64__const, ImmediateType::f64, SignatureCode::f64_, "f64.const", 0 },
    { Opcode::i32__eqz, ImmediateType::none, SignatureCode::i32__i32, "i32.eqz", 0 },
    { Opcode::i32__eq, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.eq", 0 },
    { Opcode::i32__ne, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.ne", 0 },
    { Opcode::i32__lt_s, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.lt_s", 0 },
    { Opcode::i32__lt_u, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.lt_u", 0 },
    { Opcode::i32__gt_s, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.gt_s", 0 },
    { Opcode::i32__gt_u, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.gt_u", 0 },
    { Opcode::i32__le_s, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.le_s", 0 },
    { Opcode::i32__le_u, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.le_u", 0 },
    { Opcode::i32__ge_s, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.ge_s", 0 },
    { Opcode::i32__ge_u, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.ge_u", 0 },
    { Opcode::i64__eqz, ImmediateType::none, SignatureCode::i32__i64, "i64.eqz", 0 },
    { Opcode::i64__eq, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.eq", 0 },
    { Opcode::i64__ne, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.ne", 0 },
    { Opcode::i64__lt_s, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.lt_s", 0 },
    { Opcode::i64__lt_u, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.lt_u", 0 },
    { Opcode::i64__gt_s, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.gt_s", 0 },
    { Opcode::i64__gt_u, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.gt_u", 0 },
    { Opcode::i64__le_s, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.le_s", 0 },
    { Opcode::i64__le_u, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.le_u", 0 },
    { Opcode::i64__ge_s, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.ge_s", 0 },
    { Opcode::i64__ge_u, ImmediateType::none, SignatureCode::i32__i64_i64, "i64.ge_u", 0 },
    { Opcode::f32__eq, ImmediateType::none, SignatureCode::i32__f32_f32, "f32.eq", 0 },
    { Opcode::f32__ne, ImmediateType::none, SignatureCode::i32__f32_f32, "f32.ne", 0 },
    { Opcode::f32__lt, ImmediateType::none, SignatureCode::i32__f32_f32, "f32.lt", 0 },
    { Opcode::f32__gt, ImmediateType::none, SignatureCode::i32__f32_f32, "f32.gt", 0 },
    { Opcode::f32__le, ImmediateType::none, SignatureCode::i32__f32_f32, "f32.le", 0 },
    { Opcode::f32__ge, ImmediateType::none, SignatureCode::i32__f32_f32, "f32.ge", 0 },
    { Opcode::f64__eq, ImmediateType::none, SignatureCode::i32__f64_f64, "f64.eq", 0 },
    { Opcode::f64__ne, ImmediateType::none, SignatureCode::i32__f64_f64, "f64.ne", 0 },
    { Opcode::f64__lt, ImmediateType::none, SignatureCode::i32__f64_f64, "f64.lt", 0 },
    { Opcode::f64__gt, ImmediateType::none, SignatureCode::i32__f64_f64, "f64.gt", 0 },
    { Opcode::f64__le, ImmediateType::none, SignatureCode::i32__f64_f64, "f64.le", 0 },
    { Opcode::f64__ge, ImmediateType::none, SignatureCode::i32__f64_f64, "f64.ge", 0 },
    { Opcode::i32__clz, ImmediateType::none, SignatureCode::i32__i32, "i32.clz", 0 },
    { Opcode::i32__ctz, ImmediateType::none, SignatureCode::i32__i32, "i32.ctz", 0 },
    { Opcode::i32__popcnt, ImmediateType::none, SignatureCode::i32__i32, "i32.popcnt", 0 },
    { Opcode::i32__add, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.add", 0 },
    { Opcode::i32__sub, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.sub", 0 },
    { Opcode::i32__mul, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.mul", 0 },
    { Opcode::i32__div_s, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.div_s", 0 },
    { Opcode::i32__div_u, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.div_u", 0 },
    { Opcode::i32__rem_s, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.rem_s", 0 },
    { Opcode::i32__rem_u, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.rem_u", 0 },
    { Opcode::i32__and, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.and", 0 },
    { Opcode::i32__or, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.or", 0 },
    { Opcode::i32__xor, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.xor", 0 },
    { Opcode::i32__shl, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.shl", 0 },
    { Opcode::i32__shr_s, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.shr_s", 0 },
    { Opcode::i32__shr_u, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.shr_u", 0 },
    { Opcode::i32__rotl, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.rotl", 0 },
    { Opcode::i32__rotr, ImmediateType::none, SignatureCode::i32__i32_i32, "i32.rotr", 0 },
    { Opcode::i64__clz, ImmediateType::none, SignatureCode::i64__i64, "i64.clz", 0 },
    { Opcode::i64__ctz, ImmediateType::none, SignatureCode::i64__i64, "i64.ctz", 0 },
    { Opcode::i64__popcnt, ImmediateType::none, SignatureCode::i64__i64, "i64.popcnt", 0 },
    { Opcode::i64__add, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.add", 0 },
    { Opcode::i64__sub, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.sub", 0 },
    { Opcode::i64__mul, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.mul", 0 },
    { Opcode::i64__div_s, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.div_s", 0 },
    { Opcode::i64__div_u, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.div_u", 0 },
    { Opcode::i64__rem_s, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.rem_s", 0 },
    { Opcode::i64__rem_u, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.rem_u", 0 },
    { Opcode::i64__and, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.and", 0 },
    { Opcode::i64__or, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.or", 0 },
    { Opcode::i64__xor, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.xor", 0 },
    { Opcode::i64__shl, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.shl", 0 },
    { Opcode::i64__shr_s, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.shr_s", 0 },
    { Opcode::i64__shr_u, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.shr_u", 0 },
    { Opcode::i64__rotl, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.rotl", 0 },
    { Opcode::i64__rotr, ImmediateType::none, SignatureCode::i64__i64_i64, "i64.rotr", 0 },
    { Opcode::f32__abs, ImmediateType::none, SignatureCode::f32__f32, "f32.abs", 0 },
    { Opcode::f32__neg, ImmediateType::none, SignatureCode::f32__f32, "f32.neg", 0 },
    { Opcode::f32__ceil, ImmediateType::none, SignatureCode::f32__f32, "f32.ceil", 0 },
    { Opcode::f32__floor, ImmediateType::none, SignatureCode::f32__f32, "f32.floor", 0 },
    { Opcode::f32__trunc, ImmediateType::none, SignatureCode::f32__f32, "f32.trunc", 0 },
    { Opcode::f32__nearest, ImmediateType::none, SignatureCode::f32__f32, "f32.nearest", 0 },
    { Opcode::f32__sqrt, ImmediateType::none, SignatureCode::f32__f32, "f32.sqrt", 0 },
    { Opcode::f32__add, ImmediateType::none, SignatureCode::f32__f32_f32, "f32.add", 0 },
    { Opcode::f32__sub, ImmediateType::none, SignatureCode::f32__f32_f32, "f32.sub", 0 },
    { Opcode::f32__mul, ImmediateType::none, SignatureCode::f32__f32_f32, "f32.mul", 0 },
    { Opcode::f32__div, ImmediateType::none, SignatureCode::f32__f32_f32, "f32.div", 0 },
    { Opcode::f32__min, ImmediateType::none, SignatureCode::f32__f32_f32, "f32.min", 0 },
    { Opcode::f32__max, ImmediateType::none, SignatureCode::f32__f32_f32, "f32.max", 0 },
    { Opcode::f32__copysign, ImmediateType::none, SignatureCode::f32__f32_f32, "f32.copysign", 0 },
    { Opcode::f64__abs, ImmediateType::none, SignatureCode::f64__f64, "f64.abs", 0 },
    { Opcode::f64__neg, ImmediateType::none, SignatureCode::f64__f64, "f64.neg", 0 },
    { Opcode::f64__ceil, ImmediateType::none, SignatureCode::f64__f64, "f64.ceil", 0 },
    { Opcode::f64__floor, ImmediateType::none, SignatureCode::f64__f64, "f64.floor", 0 },
    { Opcode::f64__trunc, ImmediateType::none, SignatureCode::f64__f64, "f64.trunc", 0 },
    { Opcode::f64__nearest, ImmediateType::none, SignatureCode::f64__f64, "f64.nearest", 0 },
    { Opcode::f64__sqrt, ImmediateType::none, SignatureCode::f64__f64, "f64.sqrt", 0 },
    { Opcode::f64__add, ImmediateType::none, SignatureCode::f64__f64_f64, "f64.add", 0 },
    { Opcode::f64__sub, ImmediateType::none, SignatureCode::f64__f64_f64, "f64.sub", 0 },
    { Opcode::f64__mul, ImmediateType::none, SignatureCode::f64__f64_f64, "f64.mul", 0 },
    { Opcode::f64__div, ImmediateType::none, SignatureCode::f64__f64_f64, "f64.div", 0 },
    { Opcode::f64__min, ImmediateType::none, SignatureCode::f64__f64_f64, "f64.min", 0 },
    { Opcode::f64__max, ImmediateType::none, SignatureCode::f64__f64_f64, "f64.max", 0 },
    { Opcode::f64__copysign, ImmediateType::none, SignatureCode::f64__f64_f64, "f64.copysign", 0 },
    { Opcode::i32__wrap_i64, ImmediateType::none, SignatureCode::i32__i64, "i32.wrap_i64", 0 },
    { Opcode::i32__trunc_f32_s, ImmediateType::none, SignatureCode::i32__f32, "i32.trunc_f32_s", 0 },
    { Opcode::i32__trunc_f32_u, ImmediateType::none, SignatureCode::i32__f32, "i32.trunc_f32_u", 0 },
    { Opcode::i32__trunc_f64_s, ImmediateType::none, SignatureCode::i32__f64, "i32.trunc_f64_s", 0 },
    { Opcode::i32__trunc_f64_u, ImmediateType::none, SignatureCode::i32__f64, "i32.trunc_f64_u", 0 },
    { Opcode::i64__extend_i32_s, ImmediateType::none, SignatureCode::i64__i32, "i64.extend_i32_s", 0 },
    { Opcode::i64__extend_i32_u, ImmediateType::none, SignatureCode::i64__i32, "i64.extend_i32_u", 0 },
    { Opcode::i64__trunc_f32_s, ImmediateType::none, SignatureCode::i64__f32, "i64.trunc_f32_s", 0 },
    { Opcode::i64__trunc_f32_u, ImmediateType::none, SignatureCode::i64__f32, "i64.trunc_f32_u", 0 },
    { Opcode::i64__trunc_f64_s, ImmediateType::none, SignatureCode::i64__f64, "i64.trunc_f64_s", 0 },
    { Opcode::i64__trunc_f64_u, ImmediateType::none, SignatureCode::i64__f64, "i64.trunc_f64_u", 0 },
    { Opcode::f32__convert_i32_s, ImmediateType::none, SignatureCode::f32__i32, "f32.convert_i32_s", 0 },
    { Opcode::f32__convert_i32_u, ImmediateType::none, SignatureCode::f32__i32, "f32.convert_i32_u", 0 },
    { Opcode::f32__convert_i64_s, ImmediateType::none, SignatureCode::f32__i64, "f32.convert_i64_s", 0 },
    { Opcode::f32__convert_i64_u, ImmediateType::none, SignatureCode::f32__i64, "f32.convert_i64_u", 0 },
    { Opcode::f32__demote_f64, ImmediateType::none, SignatureCode::f32__f64, "f32.demote_f64", 0 },
    { Opcode::f64__convert_i32_s, ImmediateType::none, SignatureCode::f64__i32, "f64.convert_i32_s", 0 },
    { Opcode::f64__convert_i32_u, ImmediateType::none, SignatureCode::f64__i32, "f64.convert_i32_u", 0 },
    { Opcode::f64__convert_i64_s, ImmediateType::none, SignatureCode::f64__i64, "f64.convert_i64_s", 0 },
    { Opcode::f64__convert_i64_u, ImmediateType::none, SignatureCode::f64__i64, "f64.convert_i64_u", 0 },
    { Opcode::f64__promote_f32, ImmediateType::none, SignatureCode::f64__f32, "f64.promote_f32", 0 },
    { Opcode::i32__reinterpret_f32, ImmediateType::none, SignatureCode::i32__f32, "i32.reinterpret_f32", 0 },
    { Opcode::i64__reinterpret_f64, ImmediateType::none, SignatureCode::i64__f64, "i64.reinterpret_f64", 0 },
    { Opcode::f32__reinterpret_i32, ImmediateType::none, SignatureCode::f32__i32, "f32.reinterpret_i32", 0 },
    { Opcode::f64__reinterpret_i64, ImmediateType::none, SignatureCode::f64__i64, "f64.reinterpret_i64", 0 },
    { Opcode::i32__extend8_s, ImmediateType::none, SignatureCode::i32__i32, "i32.extend8_s", 0 },
    { Opcode::i32__extend16_s, ImmediateType::none, SignatureCode::i32__i32, "i32.extend16_s", 0 },
    { Opcode::i64__extend8_s, ImmediateType::none, SignatureCode::i64__i64, "i64.extend8_s", 0 },
    { Opcode::i64__extend16_s, ImmediateType::none, SignatureCode::i64__i64, "i64.extend16_s", 0 },
    { Opcode::i64__extend32_s, ImmediateType::none, SignatureCode::i64__i64, "i64.extend32_s", 0 },
    { Opcode::ref__null, ImmediateType::refType, SignatureCode::special, "ref.null", 0 },
    { Opcode::ref__is_null, ImmediateType::refType, SignatureCode::special, "ref.is_null", 0 },
    { Opcode::ref__func, ImmediateType::functionIdx, SignatureCode::special, "ref.func", 0 },
    { Opcode::alloca, ImmediateType::none, SignatureCode::void_, "alloca", 0 },
    { Opcode::br_unless, ImmediateType::none, SignatureCode::special, "br_unless", 0 },
    { Opcode::call_host, ImmediateType::none, SignatureCode::void_, "call_host", 0 },
    { Opcode::data, ImmediateType::none, SignatureCode::void_, "data", 0 },
    { Opcode::drop_keep, ImmediateType::none, SignatureCode::void_, "drop_keep", 0 },

    // 0xfd extensions.
    { Opcode::i32__trunc_sat_f32_s, ImmediateType::none, SignatureCode::i32__f32, "i32.trunc_sat_f32_s", 0 },
    { Opcode::i32__trunc_sat_f32_u, ImmediateType::none, SignatureCode::i32__f32, "i32.trunc_sat_f32_u", 0 },
    { Opcode::i32__trunc_sat_f64_s, ImmediateType::none, SignatureCode::i32__f64, "i32.trunc_sat_f64_s", 0 },
    { Opcode::i32__trunc_sat_f64_u, ImmediateType::none, SignatureCode::i32__f64, "i32.trunc_sat_f64_u", 0 },
    { Opcode::i64__trunc_sat_f32_s, ImmediateType::none, SignatureCode::i64__f32, "i64.trunc_sat_f32_s", 0 },
    { Opcode::i64__trunc_sat_f32_u, ImmediateType::none, SignatureCode::i64__f32, "i64.trunc_sat_f32_u", 0 },
    { Opcode::i64__trunc_sat_f64_s, ImmediateType::none, SignatureCode::i64__f64, "i64.trunc_sat_f64_s", 0 },
    { Opcode::i64__trunc_sat_f64_u, ImmediateType::none, SignatureCode::i64__f64, "i64.trunc_sat_f64_u", 0 },
    { Opcode::memory__init, ImmediateType::segmentIdxMem, SignatureCode::void__i32_i32_i32, "memory.init", 0 },
    { Opcode::data__drop, ImmediateType::segmentIdx, SignatureCode::void_, "data.drop", 0 },
    { Opcode::memory__copy, ImmediateType::memMem, SignatureCode::void__i32_i32_i32, "memory.copy", 0 },
    { Opcode::memory__fill, ImmediateType::mem, SignatureCode::void__i32_i32_i32, "memory.fill", 0 },
    { Opcode::table__init, ImmediateType::tableElementIdx, SignatureCode::void__i32_i32_i32, "table.init", 0 },
    { Opcode::elem__drop, ImmediateType::elementIdx, SignatureCode::void_, "elem.drop", 0 },
    { Opcode::table__copy, ImmediateType::tableTable, SignatureCode::void__i32_i32_i32, "table.copy", 0 },
    { Opcode::table__grow, ImmediateType::table, SignatureCode::special, "table.grow", 0 },
    { Opcode::table__size, ImmediateType::table, SignatureCode::i32_, "table.size", 0 },
    { Opcode::table__fill, ImmediateType::table, SignatureCode::special, "table.fill", 0 },

    // SIMD
    { Opcode::v128__load, ImmediateType::memory, SignatureCode::v128__i32, "v128.load", 16 },
    { Opcode::i16x8__load8x8_s, ImmediateType::memory, SignatureCode::v128__i32, "i16x8.load8x8_s", 8 },
    { Opcode::i16x8__load8x8_u, ImmediateType::memory, SignatureCode::v128__i32, "i16x8.load8x8_u", 8 },
    { Opcode::i32x4__load16x4_s, ImmediateType::memory, SignatureCode::v128__i32, "i32x4.load16x4_s", 8 },
    { Opcode::i32x4__load16x4_u, ImmediateType::memory, SignatureCode::v128__i32, "i32x4.load16x4_u", 8 },
    { Opcode::i64x2__load32x2_s, ImmediateType::memory, SignatureCode::v128__i32, "i64x2.load32x2_s", 8 },
    { Opcode::i64x2__load32x2_u, ImmediateType::memory, SignatureCode::v128__i32, "i64x2.load32x2_u", 8 },
    { Opcode::v8x16__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v8x16.load_splat", 1 },
    { Opcode::v16x8__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v16x8.load_splat", 2 },
    { Opcode::v32x4__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v32x4.load_splat", 4 },
    { Opcode::v64x2__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v64x2.load_splat", 8 },
    { Opcode::v128__store, ImmediateType::memory, SignatureCode::void__i32_v128, "v128.store", 16 },
    { Opcode::v128__const, ImmediateType::v128, SignatureCode::v128_, "v128.const", 0 },
    { Opcode::v8x16__shuffle, ImmediateType::shuffle, SignatureCode::v128__v128_v128, "v8x16.shuffle", 0 },
    { Opcode::v8x16__swizzle, ImmediateType::none, SignatureCode::v128__v128_v128, "v8x16.swizzle", 0 },
    { Opcode::i8x16__splat, ImmediateType::none, SignatureCode::v128__i32, "i8x16.splat", 0 },
    { Opcode::i16x8__splat, ImmediateType::none, SignatureCode::v128__i32, "i16x8.splat", 0 },
    { Opcode::i32x4__splat, ImmediateType::none, SignatureCode::v128__i32, "i32x4.splat", 0 },
    { Opcode::i64x2__splat, ImmediateType::none, SignatureCode::v128__i64, "i64x2.splat", 0 },
    { Opcode::f32x4__splat, ImmediateType::none, SignatureCode::v128__f32, "f32x4.splat", 0 },
    { Opcode::f64x2__splat, ImmediateType::none, SignatureCode::v128__f64, "f64x2.splat", 0 },
    { Opcode::i8x16__extract_lane_s, ImmediateType::lane16Idx, SignatureCode::i32__v128, "i8x16.extract_lane_s", 0 },
    { Opcode::i8x16__extract_lane_u, ImmediateType::lane16Idx, SignatureCode::i32__v128, "i8x16.extract_lane_u", 0 },
    { Opcode::i8x16__replace_lane, ImmediateType::lane16Idx, SignatureCode::v128__v128_i32, "i8x16.replace_lane", 0 },
    { Opcode::i16x8__extract_lane_s, ImmediateType::lane8Idx, SignatureCode::i32__v128, "i16x8.extract_lane_s", 0 },
    { Opcode::i16x8__extract_lane_u, ImmediateType::lane8Idx, SignatureCode::i32__v128, "i16x8.extract_lane_u", 0 },
    { Opcode::i16x8__replace_lane, ImmediateType::lane8Idx, SignatureCode::v128__v128_i32, "i16x8.replace_lane", 0 },
    { Opcode::i32x4__extract_lane, ImmediateType::lane4Idx, SignatureCode::i32__v128, "i32x4.extract_lane", 0 },
    { Opcode::i32x4__replace_lane, ImmediateType::lane4Idx, SignatureCode::v128__v128_i32, "i32x4.replace_lane", 0 },
    { Opcode::i64x2__extract_lane, ImmediateType::lane2Idx, SignatureCode::i64__v128, "i64x2.extract_lane", 0 },
    { Opcode::i64x2__replace_lane, ImmediateType::lane2Idx, SignatureCode::v128__v128_i64, "i64x2.replace_lane", 0 },
    { Opcode::f32x4__extract_lane, ImmediateType::lane4Idx, SignatureCode::f32__v128, "f32x4.extract_lane", 0 },
    { Opcode::f32x4__replace_lane, ImmediateType::lane4Idx, SignatureCode::v128__v128_f32, "f32x4.replace_lane", 0 },
    { Opcode::f64x2__extract_lane, ImmediateType::lane2Idx, SignatureCode::f64__v128, "f64x2.extract_lane", 0 },
    { Opcode::f64x2__replace_lane, ImmediateType::lane2Idx, SignatureCode::v128__v128_f64, "f64x2.replace_lane", 0 },
    { Opcode::i8x16__eq, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.eq", 0 },
    { Opcode::i8x16__ne, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.ne", 0 },
    { Opcode::i8x16__lt_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.lt_s", 0 },
    { Opcode::i8x16__lt_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.lt_u", 0 },
    { Opcode::i8x16__gt_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.gt_s", 0 },
    { Opcode::i8x16__gt_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.gt_u", 0 },
    { Opcode::i8x16__le_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.le_s", 0 },
    { Opcode::i8x16__le_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.le_u", 0 },
    { Opcode::i8x16__ge_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.ge_s", 0 },
    { Opcode::i8x16__ge_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.ge_u", 0 },
    { Opcode::i16x8__eq, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.eq", 0 },
    { Opcode::i16x8__ne, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.ne", 0 },
    { Opcode::i16x8__lt_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.lt_s", 0 },
    { Opcode::i16x8__lt_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.lt_u", 0 },
    { Opcode::i16x8__gt_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.gt_s", 0 },
    { Opcode::i16x8__gt_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.gt_u", 0 },
    { Opcode::i16x8__le_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.le_s", 0 },
    { Opcode::i16x8__le_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.le_u", 0 },
    { Opcode::i16x8__ge_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.ge_s", 0 },
    { Opcode::i16x8__ge_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.ge_u", 0 },
    { Opcode::i32x4__eq, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.eq", 0 },
    { Opcode::i32x4__ne, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.ne", 0 },
    { Opcode::i32x4__lt_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.lt_s", 0 },
    { Opcode::i32x4__lt_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.lt_u", 0 },
    { Opcode::i32x4__gt_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.gt_s", 0 },
    { Opcode::i32x4__gt_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.gt_u", 0 },
    { Opcode::i32x4__le_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.le_s", 0 },
    { Opcode::i32x4__le_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.le_u", 0 },
    { Opcode::i32x4__ge_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.ge_s", 0 },
    { Opcode::i32x4__ge_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.ge_u", 0 },
    { Opcode::f32x4__eq, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.eq", 0 },
    { Opcode::f32x4__ne, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.ne", 0 },
    { Opcode::f32x4__lt, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.lt", 0 },
    { Opcode::f32x4__gt, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.gt", 0 },
    { Opcode::f32x4__le, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.le", 0 },
    { Opcode::f32x4__ge, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.ge", 0 },
    { Opcode::f64x2__eq, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.eq", 0 },
    { Opcode::f64x2__ne, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.ne", 0 },
    { Opcode::f64x2__lt, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.lt", 0 },
    { Opcode::f64x2__gt, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.gt", 0 },
    { Opcode::f64x2__le, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.le", 0 },
    { Opcode::f64x2__ge, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.ge", 0 },
    { Opcode::v128__not, ImmediateType::none, SignatureCode::v128__v128, "v128.not", 0 },
    { Opcode::v128__and, ImmediateType::none, SignatureCode::v128__v128_v128, "v128.and", 0 },
    { Opcode::v128__andnot, ImmediateType::none, SignatureCode::v128__v128_v128, "v128.andnot", 0 },
    { Opcode::v128__or, ImmediateType::none, SignatureCode::v128__v128_v128, "v128.or", 0 },
    { Opcode::v128__xor, ImmediateType::none, SignatureCode::v128__v128_v128, "v128.xor", 0 },
    { Opcode::v128__bitselect, ImmediateType::none, SignatureCode::v128__v128_v128_v128, "v128.bitselect", 0 },
    { Opcode::i8x16__abs, ImmediateType::none, SignatureCode::v128__v128, "i8x16.abs", 0 },
    { Opcode::i8x16__neg, ImmediateType::none, SignatureCode::v128__v128, "i8x16.neg", 0 },
    { Opcode::i8x16__any_true, ImmediateType::none, SignatureCode::i32__v128, "i8x16.any_true", 0 },
    { Opcode::i8x16__all_true, ImmediateType::none, SignatureCode::i32__v128, "i8x16.all_true", 0 },
    { Opcode::i8x16__narrow_i16x8_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.narrow_i16x8_s", 0 },
    { Opcode::i8x16__narrow_i16x8_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.narrow_i16x8_u", 0 },
    { Opcode::i8x16__shl, ImmediateType::none, SignatureCode::v128__v128_i32, "i8x16.shl", 0 },
    { Opcode::i8x16__shr_s, ImmediateType::none, SignatureCode::v128__v128_i32, "i8x16.shr_s", 0 },
    { Opcode::i8x16__shr_u, ImmediateType::none, SignatureCode::v128__v128_i32, "i8x16.shr_u", 0 },
    { Opcode::i8x16__add, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.add", 0 },
    { Opcode::i8x16__add_saturate_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.add_saturate_s", 0 },
    { Opcode::i8x16__add_saturate_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.add_saturate_u", 0 },
    { Opcode::i8x16__sub, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.sub", 0 },
    { Opcode::i8x16__sub_saturate_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.sub_saturate_s", 0 },
    { Opcode::i8x16__sub_saturate_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.sub_saturate_u", 0 },
    { Opcode::i8x16__min_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.min_s", 0 },
    { Opcode::i8x16__min_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.min_u", 0 },
    { Opcode::i8x16__max_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.max_s", 0 },
    { Opcode::i8x16__max_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.max_u", 0 },
    { Opcode::i8x16__avgr_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.avgr_u", 0 },
    { Opcode::i16x8__abs, ImmediateType::none, SignatureCode::v128__v128, "i16x8.abs", 0 },
    { Opcode::i16x8__neg, ImmediateType::none, SignatureCode::v128__v128, "i16x8.neg", 0 },
    { Opcode::i16x8__any_true, ImmediateType::none, SignatureCode::i32__v128, "i16x8.any_true", 0 },
    { Opcode::i16x8__all_true, ImmediateType::none, SignatureCode::i32__v128, "i16x8.all_true", 0 },
    { Opcode::i16x8__narrow_i32x4_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.narrow_i32x4_s", 0 },
    { Opcode::i16x8__narrow_i32x4_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.narrow_i32x4_u", 0 },
    { Opcode::i16x8__widen_low_i8x16_s, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_low_i8x16_s", 0 },
    { Opcode::i16x8__widen_high_i8x16_s, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_high_i8x16_s", 0 },
    { Opcode::i16x8__widen_low_i8x16_u, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_low_i8x16_u", 0 },
    { Opcode::i16x8__widen_high_i8x16_u, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_high_i8x16_u", 0 },
    { Opcode::i16x8__shl, ImmediateType::none, SignatureCode::v128__v128_i32, "i16x8.shl", 0 },
    { Opcode::i16x8__shr_s, ImmediateType::none, SignatureCode::v128__v128_i32, "i16x8.shr_s", 0 },
    { Opcode::i16x8__shr_u, ImmediateType::none, SignatureCode::v128__v128_i32, "i16x8.shr_u", 0 },
    { Opcode::i16x8__add, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.add", 0 },
    { Opcode::i16x8__add_saturate_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.add_saturate_s", 0 },
    { Opcode::i16x8__add_saturate_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.add_saturate_u", 0 },
    { Opcode::i16x8__sub, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.sub", 0 },
    { Opcode::i16x8__sub_saturate_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.sub_saturate_s", 0 },
    { Opcode::i16x8__sub_saturate_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.sub_saturate_u", 0 },
    { Opcode::i16x8__mul, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.mul", 0 },
    { Opcode::i16x8__min_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.min_s", 0 },
    { Opcode::i16x8__min_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.min_u", 0 },
    { Opcode::i16x8__max_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.max_s", 0 },
    { Opcode::i16x8__max_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.max_u", 0 },
    { Opcode::i16x8__avgr_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.avgr_u", 0 },
    { Opcode::i32x4__abs, ImmediateType::none, SignatureCode::v128__v128, "i32x4.abs", 0 },
    { Opcode::i32x4__neg, ImmediateType::none, SignatureCode::v128__v128, "i32x4.neg", 0 },
    { Opcode::i32x4__any_true, ImmediateType::none, SignatureCode::i32__v128, "i32x4.any_true", 0 },
    { Opcode::i32x4__all_true, ImmediateType::none, SignatureCode::i32__v128, "i32x4.all_true", 0 },
    { Opcode::i32x4__widen_low_i16x8_s, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_low_i16x8_s", 0 },
    { Opcode::i32x4__widen_high_i16x8_s, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_high_i16x8_s", 0 },
    { Opcode::i32x4__widen_low_i16x8_u, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_low_i16x8_u", 0 },
    { Opcode::i32x4__widen_high_i16x8_u, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_high_i16x8_u", 0 },
    { Opcode::i32x4__shl, ImmediateType::none, SignatureCode::v128__v128_i32, "i32x4.shl", 0 },
    { Opcode::i32x4__shr_s, ImmediateType::none, SignatureCode::v128__v128_i32, "i32x4.shr_s", 0 },
    { Opcode::i32x4__shr_u, ImmediateType::none, SignatureCode::v128__v128_i32, "i32x4.shr_u", 0 },
    { Opcode::i32x4__add, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.add", 0 },
    { Opcode::i32x4__sub, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.sub", 0 },
    { Opcode::i32x4__mul, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.mul", 0 },
    { Opcode::i32x4__min_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.min_s", 0 },
    { Opcode::i32x4__min_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.min_u", 0 },
    { Opcode::i32x4__max_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.max_s", 0 },
    { Opcode::i32x4__max_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i32x4.max_u", 0 },
    { Opcode::i64x2__neg, ImmediateType::none, SignatureCode::v128__v128, "i64x2.neg", 0 },
    { Opcode::i64x2__shl, ImmediateType::none, SignatureCode::v128__v128_i32, "i64x2.shl", 0 },
    { Opcode::i64x2__shr_s, ImmediateType::none, SignatureCode::v128__v128_i32, "i64x2.shr_s", 0 },
    { Opcode::i64x2__shr_u, ImmediateType::none, SignatureCode::v128__v128_i32, "i64x2.shr_u", 0 },
    { Opcode::i64x2__add, ImmediateType::none, SignatureCode::v128__v128_v128, "i64x2.add", 0 },
    { Opcode::i64x2__sub, ImmediateType::none, SignatureCode::v128__v128_v128, "i64x2.sub", 0 },
    { Opcode::i64x2__mul, ImmediateType::none, SignatureCode::v128__v128_v128, "i64x2.mul", 0 },
    { Opcode::f32x4__abs, ImmediateType::none, SignatureCode::v128__v128, "f32x4.abs", 0 },
    { Opcode::f32x4__neg, ImmediateType::none, SignatureCode::v128__v128, "f32x4.neg", 0 },
    { Opcode::f32x4__sqrt, ImmediateType::none, SignatureCode::v128__v128, "f32x4.sqrt", 0 },
    { Opcode::f32x4__add, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.add", 0 },
    { Opcode::f32x4__sub, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.sub", 0 },
    { Opcode::f32x4__mul, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.mul", 0 },
    { Opcode::f32x4__div, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.div", 0 },
    { Opcode::f32x4__min, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.min", 0 },
    { Opcode::f32x4__max, ImmediateType::none, SignatureCode::v128__v128_v128, "f32x4.max", 0 },
    { Opcode::f64x2__abs, ImmediateType::none, SignatureCode::v128__v128, "f64x2.abs", 0 },
    { Opcode::f64x2__neg, ImmediateType::none, SignatureCode::v128__v128, "f64x2.neg", 0 },
    { Opcode::f64x2__sqrt, ImmediateType::none, SignatureCode::v128__v128, "f64x2.sqrt", 0 },
    { Opcode::f64x2__add, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.add", 0 },
    { Opcode::f64x2__sub, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.sub", 0 },
    { Opcode::f64x2__mul, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.mul", 0 },
    { Opcode::f64x2__div, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.div", 0 },
    { Opcode::f64x2__min, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.min", 0 },
    { Opcode::f64x2__max, ImmediateType::none, SignatureCode::v128__v128_v128, "f64x2.max", 0 },
    { Opcode::i32x4__trunc_sat_f32x4_s, ImmediateType::none, SignatureCode::v128__v128, "i32x4.trunc_sat_f32x4_s", 0 },
    { Opcode::i32x4__trunc_sat_f32x4_u, ImmediateType::none, SignatureCode::v128__v128, "i32x4.trunc_sat_f32x4_u", 0 },
    { Opcode::f32x4__convert_i32x4_s, ImmediateType::none, SignatureCode::v128__v128, "f32x4.convert_i32x4_s", 0 },
    { Opcode::f32x4__convert_i32x4_u, ImmediateType::none, SignatureCode::v128__v128, "f32x4.convert_i32x4_u", 0 },

    // THREAD
    { Opcode::atomic__notify, ImmediateType::memory, SignatureCode::i32__i32_i32, "atomic.notify", 4 },
    { Opcode::i32__atomic__wait, ImmediateType::memory, SignatureCode::i32__i32_i32_i64, "i32.atomic.wait", 4 },
    { Opcode::i64__atomic__wait, ImmediateType::memory, SignatureCode::i32__i32_i64_i64, "i64.atomic.wait", 8 },
    { Opcode::i32__atomic__load, ImmediateType::memory, SignatureCode::i32__i32, "i32.atomic.load", 4 },
    { Opcode::i64__atomic__load, ImmediateType::memory, SignatureCode::i64__i32, "i64.atomic.load", 8 },
    { Opcode::i32__atomic__load8_u, ImmediateType::memory, SignatureCode::i32__i32, "i32.atomic.load8_u", 1 },
    { Opcode::i32__atomic__load16_u, ImmediateType::memory, SignatureCode::i32__i32, "i32.atomic.load16_u", 2 },
    { Opcode::i64__atomic__load8_u, ImmediateType::memory, SignatureCode::i64__i32, "i64.atomic.load8_u", 1 },
    { Opcode::i64__atomic__load16_u, ImmediateType::memory, SignatureCode::i64__i32, "i64.atomic.load16_u", 2 },
    { Opcode::i64__atomic__load32_u, ImmediateType::memory, SignatureCode::i64__i32, "i64.atomic.load32_u", 4 },
    { Opcode::i32__atomic__store, ImmediateType::memory, SignatureCode::void__i32_i32, "i32.atomic.store", 4 },
    { Opcode::i64__atomic__store, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.atomic.store", 8 },
    { Opcode::i32__atomic__store8, ImmediateType::memory, SignatureCode::void__i32_i32, "i32.atomic.store8", 1 },
    { Opcode::i32__atomic__store16, ImmediateType::memory, SignatureCode::void__i32_i32, "i32.atomic.store16", 2 },
    { Opcode::i64__atomic__store8, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.atomic.store8", 1 },
    { Opcode::i64__atomic__store16, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.atomic.store16", 2 },
    { Opcode::i64__atomic__store32, ImmediateType::memory, SignatureCode::void__i32_i64, "i64.atomic.store32", 4 },
    { Opcode::i32__atomic__rmw__add, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.add", 4 },
    { Opcode::i64__atomic__rmw__add, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.add", 8 },
    { Opcode::i32__atomic__rmw8__add_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.add_u", 1 },
    { Opcode::i32__atomic__rmw16__add_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.add_u", 2 },
    { Opcode::i64__atomic__rmw8__add_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.add_u", 1 },
    { Opcode::i64__atomic__rmw16__add_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.add_u", 2 },
    { Opcode::i64__atomic__rmw32__add_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.add_u", 4 },
    { Opcode::i32__atomic__rmw__sub, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.sub", 4 },
    { Opcode::i64__atomic__rmw__sub, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.sub", 8 },
    { Opcode::i32__atomic__rmw8__sub_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.sub_u", 1 },
    { Opcode::i32__atomic__rmw16__sub_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.sub_u", 2 },
    { Opcode::i64__atomic__rmw8__sub_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.sub_u", 1 },
    { Opcode::i64__atomic__rmw16__sub_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.sub_u", 2 },
    { Opcode::i64__atomic__rmw32__sub_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.sub_u", 4 },
    { Opcode::i32__atomic__rmw__and, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.and", 4 },
    { Opcode::i64__atomic__rmw__and, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.and", 8 },
    { Opcode::i32__atomic__rmw8__and_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.and_u", 1 },
    { Opcode::i32__atomic__rmw16__and_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.and_u", 2 },
    { Opcode::i64__atomic__rmw8__and_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.and_u", 1 },
    { Opcode::i64__atomic__rmw16__and_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.and_u", 2 },
    { Opcode::i64__atomic__rmw32__and_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.and_u", 4 },
    { Opcode::i32__atomic__rmw__or, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.or", 4 },
    { Opcode::i64__atomic__rmw__or, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.or", 8 },
    { Opcode::i32__atomic__rmw8__or_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.or_u", 1 },
    { Opcode::i32__atomic__rmw16__or_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.or_u", 2 },
    { Opcode::i64__atomic__rmw8__or_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.or_u", 1 },
    { Opcode::i64__atomic__rmw16__or_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.or_u", 2 },
    { Opcode::i64__atomic__rmw32__or_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.or_u", 4 },
    { Opcode::i32__atomic__rmw__xor, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.xor", 4 },
    { Opcode::i64__atomic__rmw__xor, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.xor", 8 },
    { Opcode::i32__atomic__rmw8__xor_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.xor_u", 1 },
    { Opcode::i32__atomic__rmw16__xor_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.xor_u", 2 },
    { Opcode::i64__atomic__rmw8__xor_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.xor_u", 1 },
    { Opcode::i64__atomic__rmw16__xor_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.xor_u", 2 },
    { Opcode::i64__atomic__rmw32__xor_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.xor_u", 4 },
    { Opcode::i32__atomic__rmw__xchg, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.xchg", 4 },
    { Opcode::i64__atomic__rmw__xchg, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.xchg", 8 },
    { Opcode::i32__atomic__rmw8__xchg_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.xchg_u", 1 },
    { Opcode::i32__atomic__rmw16__xchg_u, ImmediateType::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.xchg_u", 2 },
    { Opcode::i64__atomic__rmw8__xchg_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.xchg_u", 1 },
    { Opcode::i64__atomic__rmw16__xchg_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.xchg_u", 2 },
    { Opcode::i64__atomic__rmw32__xchg_u, ImmediateType::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.xchg_u", 4 },
    { Opcode::i32__atomic__rmw__cmpxchg, ImmediateType::memory, SignatureCode::i32__i32_i32_i32, "i32.atomic.rmw.cmpxchg", 4 },
    { Opcode::i64__atomic__rmw__cmpxchg, ImmediateType::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw.cmpxchg", 8 },
    { Opcode::i32__atomic__rmw8__cmpxchg_u, ImmediateType::memory, SignatureCode::i32__i32_i32_i32, "i32.atomic.rmw8.cmpxchg_u", 1 },
    { Opcode::i32__atomic__rmw16__cmpxchg_u, ImmediateType::memory, SignatureCode::i32__i32_i32_i32, "i32.atomic.rmw16.cmpxchg_u", 2 },
    { Opcode::i64__atomic__rmw8__cmpxchg_u, ImmediateType::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw8.cmpxchg_u", 1 },
    { Opcode::i64__atomic__rmw16__cmpxchg_u, ImmediateType::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw16.cmpxchg_u", 2 },
    { Opcode::i64__atomic__rmw32__cmpxchg_u, ImmediateType::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw32.cmpxchg_u", 4 },
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
            i32 = -0x01,       // 0x7f
            i64 = -0x02,       // 0x7e
            f32 = -0x03,       // 0x7d
            f64 = -0x04,       // 0x7c
            v128 = -0x05,      // 0x7b
            i8 = -0x06,        // 0x7a
            i16 = -0x07,       // 0x79
            funcref = -0x10,   // 0x70
            externref = -0x11, // 0x6f
            nullref = -0x12,   // 0c6e
            exnref = -0x18,    // 0x68
            func = -0x20,      // 0x60
            struct_ = -0x21,   // 0x5f
            array = -0x22,     // 0x5e
            void_ = -0x40,     // 0x40
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
        std::string_view getCName() const;
        std::string_view getRefName() const;
        std::string_view getCNullValue() const;
        
        static std::optional<ValueType> getEncoding(std::string_view v);
        static std::optional<ValueType> getRefEncoding(std::string_view v);

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
  SegmentFlagDeclared = SegmentFlags(SegmentFlagPassive | SegmentFlagExplicitIndex),
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
