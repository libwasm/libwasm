// Encodings.cpp

#include "Encodings.h"

#include <algorithm>

std::vector<Opcode::Entry> Opcode::map;

static const uint32_t simd = OpcodePrefix::simd << 24;
static const uint32_t thread = OpcodePrefix::thread << 24;

Opcode::Info Opcode::info[] =
{
    { Opcode::unreachable, ParameterEncoding::none, SignatureCode::void_, "unreachable", 0 },
    { Opcode::nop, ParameterEncoding::none, SignatureCode::void_, "nop", 0 },
    { Opcode::block, ParameterEncoding::block, SignatureCode::void_, "block", 0 },
    { Opcode::loop, ParameterEncoding::block, SignatureCode::void_, "loop", 0 },
    { Opcode::if_, ParameterEncoding::block, SignatureCode::void_, "if", 0 },
    { Opcode::else_, ParameterEncoding::none, SignatureCode::void_, "else", 0 },
    { Opcode::try_, ParameterEncoding::none, SignatureCode::void_, "try", 0 },
    { Opcode::catch_, ParameterEncoding::none, SignatureCode::void_, "catch", 0 },
    { Opcode::throw_, ParameterEncoding::none, SignatureCode::void_, "throw", 0 },
    { Opcode::rethrow_, ParameterEncoding::none, SignatureCode::void_, "rethrow", 0 },
    { Opcode::br_on_exn, ParameterEncoding::none, SignatureCode::void_, "br_on_exn", 0 },
    { Opcode::end, ParameterEncoding::none, SignatureCode::void_, "end", 0 },
    { Opcode::br, ParameterEncoding::labelIdx, SignatureCode::void_, "br", 0 },
    { Opcode::br_if, ParameterEncoding::labelIdx, SignatureCode::void_, "br_if", 0 },
    { Opcode::br_table, ParameterEncoding::table, SignatureCode::void_, "br_table", 0 },
    { Opcode::return_, ParameterEncoding::none, SignatureCode::void_, "return", 0 },
    { Opcode::call, ParameterEncoding::functionIdx, SignatureCode::void_, "call", 0 },
    { Opcode::call_indirect, ParameterEncoding::indirect, SignatureCode::void_, "call_indirect", 0 },
    { Opcode::return_call, ParameterEncoding::none, SignatureCode::void_, "return_call", 0 },
    { Opcode::return_call_indirect, ParameterEncoding::none, SignatureCode::void_, "return_call_indirect", 0 },
    { Opcode::drop, ParameterEncoding::none, SignatureCode::void_, "drop", 0 },
    { Opcode::select, ParameterEncoding::none, SignatureCode::void_, "select", 0 },
    { Opcode::local__get, ParameterEncoding::localIdx, SignatureCode::void_, "local.get", 0 },
    { Opcode::local__set, ParameterEncoding::localIdx, SignatureCode::void_, "local.set", 0 },
    { Opcode::local__tee, ParameterEncoding::localIdx, SignatureCode::void_, "local.tee", 0 },
    { Opcode::global__get, ParameterEncoding::globalIdx, SignatureCode::void_, "global.get", 0 },
    { Opcode::global__set, ParameterEncoding::globalIdx, SignatureCode::void_, "global.set", 0 },
    { Opcode::i32__load, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.load", 4 },
    { Opcode::i64__load, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.load", 8 },
    { Opcode::f32__load, ParameterEncoding::memory, SignatureCode::f32__i32, "f32.load", 4 },
    { Opcode::f64__load, ParameterEncoding::memory, SignatureCode::f64__i32, "f64.load", 8 },
    { Opcode::i32__load8_s, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.load8_s", 1 },
    { Opcode::i32__load8_u, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.load8_u", 1 },
    { Opcode::i32__load16_s, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.load16_s", 2 },
    { Opcode::i32__load16_u, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.load16_u", 2 },
    { Opcode::i64__load8_s, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.load8_s", 1 },
    { Opcode::i64__load8_u, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.load8_u", 1 },
    { Opcode::i64__load16_s, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.load16_s", 2 },
    { Opcode::i64__load16_u, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.load16_u", 2 },
    { Opcode::i64__load32_s, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.load32_s", 4 },
    { Opcode::i64__load32_u, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.load32_u", 4 },
    { Opcode::i32__store, ParameterEncoding::memory, SignatureCode::void__i32_i32, "i32.store", 4 },
    { Opcode::i64__store, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.store", 8 },
    { Opcode::f32__store, ParameterEncoding::memory, SignatureCode::void__i32_f32, "f32.store", 4 },
    { Opcode::f64__store, ParameterEncoding::memory, SignatureCode::void__i32_f64, "f64.store", 8 },
    { Opcode::i32__store8, ParameterEncoding::memory, SignatureCode::void__i32_i32, "i32.store8", 1 },
    { Opcode::i32__store16, ParameterEncoding::memory, SignatureCode::void__i32_i32, "i32.store16", 2 },
    { Opcode::i64__store8, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.store8", 1 },
    { Opcode::i64__store16, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.store16", 2 },
    { Opcode::i64__store32, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.store32", 4 },
    { Opcode::memory__size, ParameterEncoding::memory0, SignatureCode::i32_, "memory.size", 0 },
    { Opcode::memory__grow, ParameterEncoding::memory0, SignatureCode::i32__i32, "memory.grow", 0 },
    { Opcode::i32__const, ParameterEncoding::i32, SignatureCode::i32_, "i32.const", 0 },
    { Opcode::i64__const, ParameterEncoding::i64, SignatureCode::i64_, "i64.const", 0 },
    { Opcode::f32__const, ParameterEncoding::f32, SignatureCode::f32_, "f32.const", 0 },
    { Opcode::f64__const, ParameterEncoding::f64, SignatureCode::f64_, "f64.const", 0 },
    { Opcode::i32__eqz, ParameterEncoding::none, SignatureCode::i32__i32, "i32.eqz", 0 },
    { Opcode::i32__eq, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.eq", 0 },
    { Opcode::i32__ne, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.ne", 0 },
    { Opcode::i32__lt_s, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.lt_s", 0 },
    { Opcode::i32__lt_u, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.lt_u", 0 },
    { Opcode::i32__gt_s, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.gt_s", 0 },
    { Opcode::i32__gt_u, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.gt_u", 0 },
    { Opcode::i32__le_s, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.le_s", 0 },
    { Opcode::i32__le_u, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.le_u", 0 },
    { Opcode::i32__ge_s, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.ge_s", 0 },
    { Opcode::i32__ge_u, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.ge_u", 0 },
    { Opcode::i64__eqz, ParameterEncoding::none, SignatureCode::i32__i64, "i64.eqz", 0 },
    { Opcode::i64__eq, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.eq", 0 },
    { Opcode::i64__ne, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.ne", 0 },
    { Opcode::i64__lt_s, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.lt_s", 0 },
    { Opcode::i64__lt_u, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.lt_u", 0 },
    { Opcode::i64__gt_s, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.gt_s", 0 },
    { Opcode::i64__gt_u, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.gt_u", 0 },
    { Opcode::i64__le_s, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.le_s", 0 },
    { Opcode::i64__le_u, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.le_u", 0 },
    { Opcode::i64__ge_s, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.ge_s", 0 },
    { Opcode::i64__ge_u, ParameterEncoding::none, SignatureCode::i32__i64_i64, "i64.ge_u", 0 },
    { Opcode::f32__eq, ParameterEncoding::none, SignatureCode::i32__f32_f32, "f32.eq", 0 },
    { Opcode::f32__ne, ParameterEncoding::none, SignatureCode::i32__f32_f32, "f32.ne", 0 },
    { Opcode::f32__lt, ParameterEncoding::none, SignatureCode::i32__f32_f32, "f32.lt", 0 },
    { Opcode::f32__gt, ParameterEncoding::none, SignatureCode::i32__f32_f32, "f32.gt", 0 },
    { Opcode::f32__le, ParameterEncoding::none, SignatureCode::i32__f32_f32, "f32.le", 0 },
    { Opcode::f32__ge, ParameterEncoding::none, SignatureCode::i32__f32_f32, "f32.ge", 0 },
    { Opcode::f64__eq, ParameterEncoding::none, SignatureCode::i32__f64_f64, "f64.eq", 0 },
    { Opcode::f64__ne, ParameterEncoding::none, SignatureCode::i32__f64_f64, "f64.ne", 0 },
    { Opcode::f64__lt, ParameterEncoding::none, SignatureCode::i32__f64_f64, "f64.lt", 0 },
    { Opcode::f64__gt, ParameterEncoding::none, SignatureCode::i32__f64_f64, "f64.gt", 0 },
    { Opcode::f64__le, ParameterEncoding::none, SignatureCode::i32__f64_f64, "f64.le", 0 },
    { Opcode::f64__ge, ParameterEncoding::none, SignatureCode::i32__f64_f64, "f64.ge", 0 },
    { Opcode::i32__clz, ParameterEncoding::none, SignatureCode::i32__i32, "i32.clz", 0 },
    { Opcode::i32__ctz, ParameterEncoding::none, SignatureCode::i32__i32, "i32.ctz", 0 },
    { Opcode::i32__popcnt, ParameterEncoding::none, SignatureCode::i32__i32, "i32.popcnt", 0 },
    { Opcode::i32__add, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.add", 0 },
    { Opcode::i32__sub, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.sub", 0 },
    { Opcode::i32__mul, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.mul", 0 },
    { Opcode::i32__div_s, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.div_s", 0 },
    { Opcode::i32__div_u, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.div_u", 0 },
    { Opcode::i32__rem_s, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.rem_s", 0 },
    { Opcode::i32__rem_u, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.rem_u", 0 },
    { Opcode::i32__and, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.and", 0 },
    { Opcode::i32__or, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.or", 0 },
    { Opcode::i32__xor, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.xor", 0 },
    { Opcode::i32__shl, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.shl", 0 },
    { Opcode::i32__shr_s, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.shr_s", 0 },
    { Opcode::i32__shr_u, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.shr_u", 0 },
    { Opcode::i32__rotl, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.rotl", 0 },
    { Opcode::i32__rotr, ParameterEncoding::none, SignatureCode::i32__i32_i32, "i32.rotr", 0 },
    { Opcode::i64__clz, ParameterEncoding::none, SignatureCode::i64__i64, "i64.clz", 0 },
    { Opcode::i64__ctz, ParameterEncoding::none, SignatureCode::i64__i64, "i64.ctz", 0 },
    { Opcode::i64__popcnt, ParameterEncoding::none, SignatureCode::i64__i64, "i64.popcnt", 0 },
    { Opcode::i64__add, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.add", 0 },
    { Opcode::i64__sub, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.sub", 0 },
    { Opcode::i64__mul, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.mul", 0 },
    { Opcode::i64__div_s, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.div_s", 0 },
    { Opcode::i64__div_u, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.div_u", 0 },
    { Opcode::i64__rem_s, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.rem_s", 0 },
    { Opcode::i64__rem_u, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.rem_u", 0 },
    { Opcode::i64__and, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.and", 0 },
    { Opcode::i64__or, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.or", 0 },
    { Opcode::i64__xor, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.xor", 0 },
    { Opcode::i64__shl, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.shl", 0 },
    { Opcode::i64__shr_s, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.shr_s", 0 },
    { Opcode::i64__shr_u, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.shr_u", 0 },
    { Opcode::i64__rotl, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.rotl", 0 },
    { Opcode::i64__rotr, ParameterEncoding::none, SignatureCode::i64__i64_i64, "i64.rotr", 0 },
    { Opcode::f32__abs, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.abs", 0 },
    { Opcode::f32__neg, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.neg", 0 },
    { Opcode::f32__ceil, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.ceil", 0 },
    { Opcode::f32__floor, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.floor", 0 },
    { Opcode::f32__trunc, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.trunc", 0 },
    { Opcode::f32__nearest, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.nearest", 0 },
    { Opcode::f32__sqrt, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.sqrt", 0 },
    { Opcode::f32__add, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.add", 0 },
    { Opcode::f32__sub, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.sub", 0 },
    { Opcode::f32__mul, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.mul", 0 },
    { Opcode::f32__div, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.div", 0 },
    { Opcode::f32__min, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.min", 0 },
    { Opcode::f32__max, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.max", 0 },
    { Opcode::f32__copysign, ParameterEncoding::none, SignatureCode::f32__f32_f32, "f32.copysign", 0 },
    { Opcode::f64__abs, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.abs", 0 },
    { Opcode::f64__neg, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.neg", 0 },
    { Opcode::f64__ceil, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.ceil", 0 },
    { Opcode::f64__floor, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.floor", 0 },
    { Opcode::f64__trunc, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.trunc", 0 },
    { Opcode::f64__nearest, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.nearest", 0 },
    { Opcode::f64__sqrt, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.sqrt", 0 },
    { Opcode::f64__add, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.add", 0 },
    { Opcode::f64__sub, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.sub", 0 },
    { Opcode::f64__mul, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.mul", 0 },
    { Opcode::f64__div, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.div", 0 },
    { Opcode::f64__min, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.min", 0 },
    { Opcode::f64__max, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.max", 0 },
    { Opcode::f64__copysign, ParameterEncoding::none, SignatureCode::f64__f64_f64, "f64.copysign", 0 },
    { Opcode::i32__wrap_i64, ParameterEncoding::none, SignatureCode::i32__i64, "i32.wrap_i64", 0 },
    { Opcode::i32__trunc_f32_s, ParameterEncoding::none, SignatureCode::i32__f32, "i32.trunc_f32_s", 0 },
    { Opcode::i32__trunc_f32_u, ParameterEncoding::none, SignatureCode::i32__f32, "i32.trunc_f32_u", 0 },
    { Opcode::i32__trunc_f64_s, ParameterEncoding::none, SignatureCode::i32__f64, "i32.trunc_f64_s", 0 },
    { Opcode::i32__trunc_f64_u, ParameterEncoding::none, SignatureCode::i32__f64, "i32.trunc_f64_u", 0 },
    { Opcode::i64__extend_i32_s, ParameterEncoding::none, SignatureCode::i64__i32, "i64.extend_i32_s", 0 },
    { Opcode::i64__extend_i32_u, ParameterEncoding::none, SignatureCode::i64__i32, "i64.extend_i32_u", 0 },
    { Opcode::i64__trunc_f32_s, ParameterEncoding::none, SignatureCode::i64__f32, "i64.trunc_f32_s", 0 },
    { Opcode::i64__trunc_f32_u, ParameterEncoding::none, SignatureCode::i64__f32, "i64.trunc_f32_u", 0 },
    { Opcode::i64__trunc_f64_s, ParameterEncoding::none, SignatureCode::i64__f64, "i64.trunc_f64_s", 0 },
    { Opcode::i64__trunc_f64_u, ParameterEncoding::none, SignatureCode::i64__f64, "i64.trunc_f64_u", 0 },
    { Opcode::f32__convert_i32_s, ParameterEncoding::none, SignatureCode::f32__i32, "f32.convert_i32_s", 0 },
    { Opcode::f32__convert_i32_u, ParameterEncoding::none, SignatureCode::f32__i32, "f32.convert_i32_u", 0 },
    { Opcode::f32__convert_i64_s, ParameterEncoding::none, SignatureCode::f32__i64, "f32.convert_i64_s", 0 },
    { Opcode::f32__convert_i64_u, ParameterEncoding::none, SignatureCode::f32__i64, "f32.convert_i64_u", 0 },
    { Opcode::f32__demote_f64, ParameterEncoding::none, SignatureCode::f32__f64, "f32.demote_f64", 0 },
    { Opcode::f64__convert_i32_s, ParameterEncoding::none, SignatureCode::f64__i32, "f64.convert_i32_s", 0 },
    { Opcode::f64__convert_i32_u, ParameterEncoding::none, SignatureCode::f64__i32, "f64.convert_i32_u", 0 },
    { Opcode::f64__convert_i64_s, ParameterEncoding::none, SignatureCode::f64__i64, "f64.convert_i64_s", 0 },
    { Opcode::f64__convert_i64_u, ParameterEncoding::none, SignatureCode::f64__i64, "f64.convert_i64_u", 0 },
    { Opcode::f64__promote_f32, ParameterEncoding::none, SignatureCode::f64__f32, "f64.promote_f32", 0 },
    { Opcode::i32__reinterpret_f32, ParameterEncoding::none, SignatureCode::i32__f32, "i32.reinterpret_f32", 0 },
    { Opcode::i64__reinterpret_f64, ParameterEncoding::none, SignatureCode::i64__f64, "i64.reinterpret_f64", 0 },
    { Opcode::f32__reinterpret_i32, ParameterEncoding::none, SignatureCode::f32__i32, "f32.reinterpret_i32", 0 },
    { Opcode::f64__reinterpret_i64, ParameterEncoding::none, SignatureCode::f64__i64, "f64.reinterpret_i64", 0 },
    { Opcode::i32__extend8_s, ParameterEncoding::none, SignatureCode::i32__i32, "i32.extend8_s", 0 },
    { Opcode::i32__extend16_s, ParameterEncoding::none, SignatureCode::i32__i32, "i32.extend16_s", 0 },
    { Opcode::i64__extend8_s, ParameterEncoding::none, SignatureCode::i64__i64, "i64.extend8_s", 0 },
    { Opcode::i64__extend16_s, ParameterEncoding::none, SignatureCode::i64__i64, "i64.extend16_s", 0 },
    { Opcode::i64__extend32_s, ParameterEncoding::none, SignatureCode::i64__i64, "i64.extend32_s", 0 },
    { Opcode::alloca, ParameterEncoding::none, SignatureCode::void_, "alloca", 0 },
    { Opcode::br_unless, ParameterEncoding::none, SignatureCode::void_, "br_unless", 0 },
    { Opcode::call_host, ParameterEncoding::none, SignatureCode::void_, "call_host", 0 },
    { Opcode::data, ParameterEncoding::none, SignatureCode::void_, "data", 0 },
    { Opcode::drop_keep, ParameterEncoding::none, SignatureCode::void_, "drop_keep", 0 },
    { Opcode::i32__trunc_sat_f32_s, ParameterEncoding::none, SignatureCode::i32__f32, "i32.trunc_sat_f32_s", 0 },
    { Opcode::i32__trunc_sat_f32_u, ParameterEncoding::none, SignatureCode::i32__f32, "i32.trunc_sat_f32_u", 0 },
    { Opcode::i32__trunc_sat_f64_s, ParameterEncoding::none, SignatureCode::i32__f64, "i32.trunc_sat_f64_s", 0 },
    { Opcode::i32__trunc_sat_f64_u, ParameterEncoding::none, SignatureCode::i32__f64, "i32.trunc_sat_f64_u", 0 },
    { Opcode::i64__trunc_sat_f32_s, ParameterEncoding::none, SignatureCode::i64__f32, "i64.trunc_sat_f32_s", 0 },
    { Opcode::i64__trunc_sat_f32_u, ParameterEncoding::none, SignatureCode::i64__f32, "i64.trunc_sat_f32_u", 0 },
    { Opcode::i64__trunc_sat_f64_s, ParameterEncoding::none, SignatureCode::i64__f64, "i64.trunc_sat_f64_s", 0 },
    { Opcode::i64__trunc_sat_f64_u, ParameterEncoding::none, SignatureCode::i64__f64, "i64.trunc_sat_f64_u", 0 },
    { Opcode::memory__init, ParameterEncoding::none, SignatureCode::void__i32_i32_i32, "memory.init", 0 },
    { Opcode::data__drop, ParameterEncoding::none, SignatureCode::void_, "data.drop", 0 },
    { Opcode::memory__copy, ParameterEncoding::none, SignatureCode::void__i32_i32_i32, "memory.copy", 0 },
    { Opcode::memory__fill, ParameterEncoding::none, SignatureCode::void__i32_i32_i32, "memory.fill", 0 },
    { Opcode::table__init, ParameterEncoding::none, SignatureCode::void__i32_i32_i32, "table.init", 0 },
    { Opcode::elem__drop, ParameterEncoding::none, SignatureCode::void_, "elem.drop", 0 },
    { Opcode::table__copy, ParameterEncoding::none, SignatureCode::void__i32_i32_i32, "table.copy", 0 },
    { Opcode::table__get, ParameterEncoding::none, SignatureCode::void_, "table.get", 0 },
    { Opcode::table__set, ParameterEncoding::none, SignatureCode::void_, "table.set", 0 },
    { Opcode::table__grow, ParameterEncoding::none, SignatureCode::void_, "table.grow", 0 },
    { Opcode::table__size, ParameterEncoding::none, SignatureCode::void_, "table.size", 0 },
    { Opcode::table__fill, ParameterEncoding::none, SignatureCode::void_, "table.fill", 0 },
    { Opcode::ref__null, ParameterEncoding::none, SignatureCode::void_, "ref.null", 0 },
    { Opcode::ref__is_null, ParameterEncoding::none, SignatureCode::void_, "ref.is_null", 0 },
    { Opcode::ref__func, ParameterEncoding::none, SignatureCode::void_, "ref.func", 0 },

    // SIMD
    { simd | Opcode::v128__load, ParameterEncoding::memory, SignatureCode::v128__i32, "v128.load", 16 },
    { simd | Opcode::v128__store, ParameterEncoding::memory, SignatureCode::void__i32_v128, "v128.store", 16 },
    { simd | Opcode::v128__const, ParameterEncoding::v128, SignatureCode::v128_, "v128.const", 0 },
    { simd | Opcode::i8x16__splat, ParameterEncoding::none, SignatureCode::v128__i32, "i8x16.splat", 0 },
    { simd | Opcode::i8x16__extract_lane_s, ParameterEncoding::lane16Idx, SignatureCode::i32__v128, "i8x16.extract_lane_s", 0 },
    { simd | Opcode::i8x16__extract_lane_u, ParameterEncoding::lane16Idx, SignatureCode::i32__v128, "i8x16.extract_lane_u", 0 },
    { simd | Opcode::i8x16__replace_lane, ParameterEncoding::lane16Idx, SignatureCode::v128__v128_i32, "i8x16.replace_lane", 0 },
    { simd | Opcode::i16x8__splat, ParameterEncoding::none, SignatureCode::v128__i32, "i16x8.splat", 0 },
    { simd | Opcode::i16x8__extract_lane_s, ParameterEncoding::lane8Idx, SignatureCode::i32__v128, "i16x8.extract_lane_s", 0 },
    { simd | Opcode::i16x8__extract_lane_u, ParameterEncoding::lane8Idx, SignatureCode::i32__v128, "i16x8.extract_lane_u", 0 },
    { simd | Opcode::i16x8__replace_lane, ParameterEncoding::lane8Idx, SignatureCode::v128__v128_i32, "i16x8.replace_lane", 0 },
    { simd | Opcode::i32x4__splat, ParameterEncoding::none, SignatureCode::v128__i32, "i32x4.splat", 0 },
    { simd | Opcode::i32x4__extract_lane, ParameterEncoding::lane4Idx, SignatureCode::i32__v128, "i32x4.extract_lane", 0 },
    { simd | Opcode::i32x4__replace_lane, ParameterEncoding::lane4Idx, SignatureCode::v128__v128_i32, "i32x4.replace_lane", 0 },
    { simd | Opcode::i64x2__splat, ParameterEncoding::none, SignatureCode::v128__i64, "i64x2.splat", 0 },
    { simd | Opcode::i64x2__extract_lane, ParameterEncoding::lane2Idx, SignatureCode::i64__v128, "i64x2.extract_lane", 0 },
    { simd | Opcode::i64x2__replace_lane, ParameterEncoding::lane2Idx, SignatureCode::v128__v128_i64, "i64x2.replace_lane", 0 },
    { simd | Opcode::f32x4__splat, ParameterEncoding::none, SignatureCode::v128__f32, "f32x4.splat", 0 },
    { simd | Opcode::f32x4__extract_lane, ParameterEncoding::lane4Idx, SignatureCode::f32__v128, "f32x4.extract_lane", 0 },
    { simd | Opcode::f32x4__replace_lane, ParameterEncoding::lane4Idx, SignatureCode::v128__v128_f32, "f32x4.replace_lane", 0 },
    { simd | Opcode::f64x2__splat, ParameterEncoding::none, SignatureCode::v128__f64, "f64x2.splat", 0 },
    { simd | Opcode::f64x2__extract_lane, ParameterEncoding::lane2Idx, SignatureCode::f64__v128, "f64x2.extract_lane", 0 },
    { simd | Opcode::f64x2__replace_lane, ParameterEncoding::lane2Idx, SignatureCode::v128__v128_f64, "f64x2.replace_lane", 0 },
    { simd | Opcode::i8x16__eq, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.eq", 0 },
    { simd | Opcode::i8x16__ne, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.ne", 0 },
    { simd | Opcode::i8x16__lt_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.lt_s", 0 },
    { simd | Opcode::i8x16__lt_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.lt_u", 0 },
    { simd | Opcode::i8x16__gt_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.gt_s", 0 },
    { simd | Opcode::i8x16__gt_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.gt_u", 0 },
    { simd | Opcode::i8x16__le_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.le_s", 0 },
    { simd | Opcode::i8x16__le_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.le_u", 0 },
    { simd | Opcode::i8x16__ge_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.ge_s", 0 },
    { simd | Opcode::i8x16__ge_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.ge_u", 0 },
    { simd | Opcode::i16x8__eq, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.eq", 0 },
    { simd | Opcode::i16x8__ne, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.ne", 0 },
    { simd | Opcode::i16x8__lt_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.lt_s", 0 },
    { simd | Opcode::i16x8__lt_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.lt_u", 0 },
    { simd | Opcode::i16x8__gt_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.gt_s", 0 },
    { simd | Opcode::i16x8__gt_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.gt_u", 0 },
    { simd | Opcode::i16x8__le_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.le_s", 0 },
    { simd | Opcode::i16x8__le_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.le_u", 0 },
    { simd | Opcode::i16x8__ge_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.ge_s", 0 },
    { simd | Opcode::i16x8__ge_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.ge_u", 0 },
    { simd | Opcode::i32x4__eq, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.eq", 0 },
    { simd | Opcode::i32x4__ne, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.ne", 0 },
    { simd | Opcode::i32x4__lt_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.lt_s", 0 },
    { simd | Opcode::i32x4__lt_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.lt_u", 0 },
    { simd | Opcode::i32x4__gt_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.gt_s", 0 },
    { simd | Opcode::i32x4__gt_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.gt_u", 0 },
    { simd | Opcode::i32x4__le_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.le_s", 0 },
    { simd | Opcode::i32x4__le_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.le_u", 0 },
    { simd | Opcode::i32x4__ge_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.ge_s", 0 },
    { simd | Opcode::i32x4__ge_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.ge_u", 0 },
    { simd | Opcode::f32x4__eq, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.eq", 0 },
    { simd | Opcode::f32x4__ne, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.ne", 0 },
    { simd | Opcode::f32x4__lt, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.lt", 0 },
    { simd | Opcode::f32x4__gt, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.gt", 0 },
    { simd | Opcode::f32x4__le, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.le", 0 },
    { simd | Opcode::f32x4__ge, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.ge", 0 },
    { simd | Opcode::f64x2__eq, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.eq", 0 },
    { simd | Opcode::f64x2__ne, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.ne", 0 },
    { simd | Opcode::f64x2__lt, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.lt", 0 },
    { simd | Opcode::f64x2__gt, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.gt", 0 },
    { simd | Opcode::f64x2__le, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.le", 0 },
    { simd | Opcode::f64x2__ge, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.ge", 0 },
    { simd | Opcode::v128__not, ParameterEncoding::none, SignatureCode::v128__v128, "v128.not", 0 },
    { simd | Opcode::v128__and, ParameterEncoding::none, SignatureCode::v128__v128_v128, "v128.and", 0 },
    { simd | Opcode::v128__or, ParameterEncoding::none, SignatureCode::v128__v128_v128, "v128.or", 0 },
    { simd | Opcode::v128__xor, ParameterEncoding::none, SignatureCode::v128__v128_v128, "v128.xor", 0 },
    { simd | Opcode::v128__bitselect, ParameterEncoding::none, SignatureCode::v128__v128_v128_v128, "v128.bitselect", 0 },
    { simd | Opcode::i8x16__neg, ParameterEncoding::none, SignatureCode::v128__v128, "i8x16.neg", 0 },
    { simd | Opcode::i8x16__any_true, ParameterEncoding::none, SignatureCode::i32__v128, "i8x16.any_true", 0 },
    { simd | Opcode::i8x16__all_true, ParameterEncoding::none, SignatureCode::i32__v128, "i8x16.all_true", 0 },
    { simd | Opcode::i8x16__shl, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i8x16.shl", 0 },
    { simd | Opcode::i8x16__shr_s, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i8x16.shr_s", 0 },
    { simd | Opcode::i8x16__shr_u, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i8x16.shr_u", 0 },
    { simd | Opcode::i8x16__add, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.add", 0 },
    { simd | Opcode::i8x16__add_saturate_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.add_saturate_s", 0 },
    { simd | Opcode::i8x16__add_saturate_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.add_saturate_u", 0 },
    { simd | Opcode::i8x16__sub, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.sub", 0 },
    { simd | Opcode::i8x16__sub_saturate_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.sub_saturate_s", 0 },
    { simd | Opcode::i8x16__sub_saturate_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.sub_saturate_u", 0 },
    { simd | Opcode::i8x16__mul, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.mul", 0 },
    { simd | Opcode::i16x8__neg, ParameterEncoding::none, SignatureCode::v128__v128, "i16x8.neg", 0 },
    { simd | Opcode::i16x8__any_true, ParameterEncoding::none, SignatureCode::i32__v128, "i16x8.any_true", 0 },
    { simd | Opcode::i16x8__all_true, ParameterEncoding::none, SignatureCode::i32__v128, "i16x8.all_true", 0 },
    { simd | Opcode::i16x8__shl, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i16x8.shl", 0 },
    { simd | Opcode::i16x8__shr_s, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i16x8.shr_s", 0 },
    { simd | Opcode::i16x8__shr_u, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i16x8.shr_u", 0 },
    { simd | Opcode::i16x8__add, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.add", 0 },
    { simd | Opcode::i16x8__add_saturate_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.add_saturate_s", 0 },
    { simd | Opcode::i16x8__add_saturate_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.add_saturate_u", 0 },
    { simd | Opcode::i16x8__sub, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.sub", 0 },
    { simd | Opcode::i16x8__sub_saturate_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.sub_saturate_s", 0 },
    { simd | Opcode::i16x8__sub_saturate_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.sub_saturate_u", 0 },
    { simd | Opcode::i16x8__mul, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.mul", 0 },
    { simd | Opcode::i32x4__neg, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.neg", 0 },
    { simd | Opcode::i32x4__any_true, ParameterEncoding::none, SignatureCode::i32__v128, "i32x4.any_true", 0 },
    { simd | Opcode::i32x4__all_true, ParameterEncoding::none, SignatureCode::i32__v128, "i32x4.all_true", 0 },
    { simd | Opcode::i32x4__shl, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i32x4.shl", 0 },
    { simd | Opcode::i32x4__shr_s, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i32x4.shr_s", 0 },
    { simd | Opcode::i32x4__shr_u, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i32x4.shr_u", 0 },
    { simd | Opcode::i32x4__add, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.add", 0 },
    { simd | Opcode::i32x4__sub, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.sub", 0 },
    { simd | Opcode::i32x4__mul, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i32x4.mul", 0 },
    { simd | Opcode::i64x2__neg, ParameterEncoding::none, SignatureCode::v128__v128, "i64x2.neg", 0 },
    { simd | Opcode::i64x2__any_true, ParameterEncoding::none, SignatureCode::i32__v128, "i64x2.any_true", 0 },
    { simd | Opcode::i64x2__all_true, ParameterEncoding::none, SignatureCode::i32__v128, "i64x2.all_true", 0 },
    { simd | Opcode::i64x2__shl, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i64x2.shl", 0 },
    { simd | Opcode::i64x2__shr_s, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i64x2.shr_s", 0 },
    { simd | Opcode::i64x2__shr_u, ParameterEncoding::none, SignatureCode::v128__v128_i32, "i64x2.shr_u", 0 },
    { simd | Opcode::i64x2__add, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i64x2.add", 0 },
    { simd | Opcode::i64x2__sub, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i64x2.sub", 0 },
    { simd | Opcode::f32x4__abs, ParameterEncoding::none, SignatureCode::v128__v128, "f32x4.abs", 0 },
    { simd | Opcode::f32x4__neg, ParameterEncoding::none, SignatureCode::v128__v128, "f32x4.neg", 0 },
    { simd | Opcode::f32x4__sqrt, ParameterEncoding::none, SignatureCode::v128__v128, "f32x4.sqrt", 0 },
    { simd | Opcode::f32x4__add, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.add", 0 },
    { simd | Opcode::f32x4__sub, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.sub", 0 },
    { simd | Opcode::f32x4__mul, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.mul", 0 },
    { simd | Opcode::f32x4__div, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.div", 0 },
    { simd | Opcode::f32x4__min, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.min", 0 },
    { simd | Opcode::f32x4__max, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f32x4.max", 0 },
    { simd | Opcode::f64x2__abs, ParameterEncoding::none, SignatureCode::v128__v128, "f64x2.abs", 0 },
    { simd | Opcode::f64x2__neg, ParameterEncoding::none, SignatureCode::v128__v128, "f64x2.neg", 0 },
    { simd | Opcode::f64x2__sqrt, ParameterEncoding::none, SignatureCode::v128__v128, "f64x2.sqrt", 0 },
    { simd | Opcode::f64x2__add, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.add", 0 },
    { simd | Opcode::f64x2__sub, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.sub", 0 },
    { simd | Opcode::f64x2__mul, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.mul", 0 },
    { simd | Opcode::f64x2__div, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.div", 0 },
    { simd | Opcode::f64x2__min, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.min", 0 },
    { simd | Opcode::f64x2__max, ParameterEncoding::none, SignatureCode::v128__v128_v128, "f64x2.max", 0 },
    { simd | Opcode::i32x4__trunc_sat_f32x4_s, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.trunc_sat_f32x4_s", 0 },
    { simd | Opcode::i32x4__trunc_sat_f32x4_u, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.trunc_sat_f32x4_u", 0 },
    { simd | Opcode::i64x2__trunc_sat_f64x2_s, ParameterEncoding::none, SignatureCode::v128__v128, "i64x2.trunc_sat_f64x2_s", 0 },
    { simd | Opcode::i64x2__trunc_sat_f64x2_u, ParameterEncoding::none, SignatureCode::v128__v128, "i64x2.trunc_sat_f64x2_u", 0 },
    { simd | Opcode::f32x4__convert_i32x4_s, ParameterEncoding::none, SignatureCode::v128__v128, "f32x4.convert_i32x4_s", 0 },
    { simd | Opcode::f32x4__convert_i32x4_u, ParameterEncoding::none, SignatureCode::v128__v128, "f32x4.convert_i32x4_u", 0 },
    { simd | Opcode::f64x2__convert_i64x2_s, ParameterEncoding::none, SignatureCode::v128__v128, "f64x2.convert_i64x2_s", 0 },
    { simd | Opcode::f64x2__convert_i64x2_u, ParameterEncoding::none, SignatureCode::v128__v128, "f64x2.convert_i64x2_u", 0 },
    { simd | Opcode::v8x16__swizzle, ParameterEncoding::none, SignatureCode::v128__v128_v128, "v8x16.swizzle", 0 },
    { simd | Opcode::v8x16__shuffle, ParameterEncoding::shuffle, SignatureCode::v128__v128_v128, "v8x16.shuffle", 0 },
    { simd | Opcode::i8x16__load_splat, ParameterEncoding::none, SignatureCode::v128__i32, "i8x16.load_splat", 1 },
    { simd | Opcode::i16x8__load_splat, ParameterEncoding::none, SignatureCode::v128__i32, "i16x8.load_splat", 2 },
    { simd | Opcode::i32x4__load_splat, ParameterEncoding::none, SignatureCode::v128__i32, "i32x4.load_splat", 4 },
    { simd | Opcode::i64x2__load_splat, ParameterEncoding::none, SignatureCode::v128__i32, "i64x2.load_splat", 8 },
    { simd | Opcode::i8x16__narrow_i16x8_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.narrow_i16x8_s", 0 },
    { simd | Opcode::i8x16__narrow_i16x8_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.narrow_i16x8_u", 0 },
    { simd | Opcode::i16x8__narrow_i32x4_s, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.narrow_i32x4_s", 0 },
    { simd | Opcode::i16x8__narrow_i32x4_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.narrow_i32x4_u", 0 },
    { simd | Opcode::i16x8__widen_low_i8x16_s, ParameterEncoding::none, SignatureCode::v128__v128, "i16x8.widen_low_i8x16_s", 0 },
    { simd | Opcode::i16x8__widen_high_i8x16_s, ParameterEncoding::none, SignatureCode::v128__v128, "i16x8.widen_high_i8x16_s", 0 },
    { simd | Opcode::i16x8__widen_low_i8x16_u, ParameterEncoding::none, SignatureCode::v128__v128, "i16x8.widen_low_i8x16_u", 0 },
    { simd | Opcode::i16x8__widen_high_i8x16_u, ParameterEncoding::none, SignatureCode::v128__v128, "i16x8.widen_high_i8x16_u", 0 },
    { simd | Opcode::i32x4__widen_low_i16x8_s, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.widen_low_i16x8_s", 0 },
    { simd | Opcode::i32x4__widen_high_i16x8_s, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.widen_high_i16x8_s", 0 },
    { simd | Opcode::i32x4__widen_low_i16x8_u, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.widen_low_i16x8_u", 0 },
    { simd | Opcode::i32x4__widen_high_i16x8_u, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.widen_high_i16x8_u", 0 },
    { simd | Opcode::i16x8__load8x8_s, ParameterEncoding::memory, SignatureCode::v128_, "i16x8.load8x8_s", 2 },
    { simd | Opcode::i16x8__load8x8_u, ParameterEncoding::memory, SignatureCode::v128_, "i16x8.load8x8_u", 2 },
    { simd | Opcode::i32x4__load16x4_s, ParameterEncoding::memory, SignatureCode::v128_, "i32x4.load16x4_s", 4 },
    { simd | Opcode::i32x4__load16x4_u, ParameterEncoding::memory, SignatureCode::v128_, "i32x4.load16x4_u", 4 },
    { simd | Opcode::i64x2__load32x2_s, ParameterEncoding::memory, SignatureCode::v128_, "i64x2.load32x2_s", 8 },
    { simd | Opcode::i64x2__load32x2_u, ParameterEncoding::memory, SignatureCode::v128_, "i64x2.load32x2_u", 8 },
    { simd | Opcode::v128__andnot, ParameterEncoding::none, SignatureCode::v128__v128_v128, "v128.andnot", 0 },
    { simd | Opcode::i8x16__avgr_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i8x16.avgr_u", 0 },
    { simd | Opcode::i16x8__avgr_u, ParameterEncoding::none, SignatureCode::v128__v128_v128, "i16x8.avgr_u", 0 },
    { simd | Opcode::i8x16__abs, ParameterEncoding::none, SignatureCode::v128__v128, "i8x16.abs", 0 },
    { simd | Opcode::i16x8__abs, ParameterEncoding::none, SignatureCode::v128__v128, "i16x8.abs", 0 },
    { simd | Opcode::i32x4__abs, ParameterEncoding::none, SignatureCode::v128__v128, "i32x4.abs", 0 },

    // THREAD
    { thread | Opcode::atomic__notify, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "atomic.notify", 4 },
    { thread | Opcode::i32__atomic__wait, ParameterEncoding::memory, SignatureCode::i32__i32_i32_i64, "i32.atomic.wait", 4 },
    { thread | Opcode::i64__atomic__wait, ParameterEncoding::memory, SignatureCode::i32__i32_i64_i64, "i64.atomic.wait", 8 },
    { thread | Opcode::i32__atomic__load, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.atomic.load", 4 },
    { thread | Opcode::i64__atomic__load, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.atomic.load", 8 },
    { thread | Opcode::i32__atomic__load8_u, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.atomic.load8_u", 1 },
    { thread | Opcode::i32__atomic__load16_u, ParameterEncoding::memory, SignatureCode::i32__i32, "i32.atomic.load16_u", 2 },
    { thread | Opcode::i64__atomic__load8_u, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.atomic.load8_u", 1 },
    { thread | Opcode::i64__atomic__load16_u, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.atomic.load16_u", 2 },
    { thread | Opcode::i64__atomic__load32_u, ParameterEncoding::memory, SignatureCode::i64__i32, "i64.atomic.load32_u", 4 },
    { thread | Opcode::i32__atomic__store, ParameterEncoding::memory, SignatureCode::void__i32_i32, "i32.atomic.store", 4 },
    { thread | Opcode::i64__atomic__store, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.atomic.store", 8 },
    { thread | Opcode::i32__atomic__store8, ParameterEncoding::memory, SignatureCode::void__i32_i32, "i32.atomic.store8", 1 },
    { thread | Opcode::i32__atomic__store16, ParameterEncoding::memory, SignatureCode::void__i32_i32, "i32.atomic.store16", 2 },
    { thread | Opcode::i64__atomic__store8, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.atomic.store8", 1 },
    { thread | Opcode::i64__atomic__store16, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.atomic.store16", 2 },
    { thread | Opcode::i64__atomic__store32, ParameterEncoding::memory, SignatureCode::void__i32_i64, "i64.atomic.store32", 4 },
    { thread | Opcode::i32__atomic__rmw__add, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.add", 4 },
    { thread | Opcode::i64__atomic__rmw__add, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.add", 8 },
    { thread | Opcode::i32__atomic__rmw8__add_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.add_u", 1 },
    { thread | Opcode::i32__atomic__rmw16__add_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.add_u", 2 },
    { thread | Opcode::i64__atomic__rmw8__add_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.add_u", 1 },
    { thread | Opcode::i64__atomic__rmw16__add_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.add_u", 2 },
    { thread | Opcode::i64__atomic__rmw32__add_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.add_u", 4 },
    { thread | Opcode::i32__atomic__rmw__sub, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.sub", 4 },
    { thread | Opcode::i64__atomic__rmw__sub, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.sub", 8 },
    { thread | Opcode::i32__atomic__rmw8__sub_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.sub_u", 1 },
    { thread | Opcode::i32__atomic__rmw16__sub_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.sub_u", 2 },
    { thread | Opcode::i64__atomic__rmw8__sub_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.sub_u", 1 },
    { thread | Opcode::i64__atomic__rmw16__sub_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.sub_u", 2 },
    { thread | Opcode::i64__atomic__rmw32__sub_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.sub_u", 4 },
    { thread | Opcode::i32__atomic__rmw__and, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.and", 4 },
    { thread | Opcode::i64__atomic__rmw__and, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.and", 8 },
    { thread | Opcode::i32__atomic__rmw8__and_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.and_u", 1 },
    { thread | Opcode::i32__atomic__rmw16__and_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.and_u", 2 },
    { thread | Opcode::i64__atomic__rmw8__and_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.and_u", 1 },
    { thread | Opcode::i64__atomic__rmw16__and_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.and_u", 2 },
    { thread | Opcode::i64__atomic__rmw32__and_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.and_u", 4 },
    { thread | Opcode::i32__atomic__rmw__or, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.or", 4 },
    { thread | Opcode::i64__atomic__rmw__or, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.or", 8 },
    { thread | Opcode::i32__atomic__rmw8__or_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.or_u", 1 },
    { thread | Opcode::i32__atomic__rmw16__or_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.or_u", 2 },
    { thread | Opcode::i64__atomic__rmw8__or_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.or_u", 1 },
    { thread | Opcode::i64__atomic__rmw16__or_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.or_u", 2 },
    { thread | Opcode::i64__atomic__rmw32__or_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.or_u", 4 },
    { thread | Opcode::i32__atomic__rmw__xor, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.xor", 4 },
    { thread | Opcode::i64__atomic__rmw__xor, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.xor", 8 },
    { thread | Opcode::i32__atomic__rmw8__xor_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.xor_u", 1 },
    { thread | Opcode::i32__atomic__rmw16__xor_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.xor_u", 2 },
    { thread | Opcode::i64__atomic__rmw8__xor_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.xor_u", 1 },
    { thread | Opcode::i64__atomic__rmw16__xor_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.xor_u", 2 },
    { thread | Opcode::i64__atomic__rmw32__xor_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.xor_u", 4 },
    { thread | Opcode::i32__atomic__rmw__xchg, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw.xchg", 4 },
    { thread | Opcode::i64__atomic__rmw__xchg, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw.xchg", 8 },
    { thread | Opcode::i32__atomic__rmw8__xchg_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw8.xchg_u", 1 },
    { thread | Opcode::i32__atomic__rmw16__xchg_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32, "i32.atomic.rmw16.xchg_u", 2 },
    { thread | Opcode::i64__atomic__rmw8__xchg_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw8.xchg_u", 1 },
    { thread | Opcode::i64__atomic__rmw16__xchg_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw16.xchg_u", 2 },
    { thread | Opcode::i64__atomic__rmw32__xchg_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64, "i64.atomic.rmw32.xchg_u", 4 },
    { thread | Opcode::i32__atomic__rmw__cmpxchg, ParameterEncoding::memory, SignatureCode::i32__i32_i32_i32, "i32.atomic.rmw.cmpxchg", 4 },
    { thread | Opcode::i64__atomic__rmw__cmpxchg, ParameterEncoding::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw.cmpxchg", 8 },
    { thread | Opcode::i32__atomic__rmw8__cmpxchg_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32_i32, "i32.atomic.rmw8.cmpxchg_u", 1 },
    { thread | Opcode::i32__atomic__rmw16__cmpxchg_u, ParameterEncoding::memory, SignatureCode::i32__i32_i32_i32, "i32.atomic.rmw16.cmpxchg_u", 2 },
    { thread | Opcode::i64__atomic__rmw8__cmpxchg_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw8.cmpxchg_u", 1 },
    { thread | Opcode::i64__atomic__rmw16__cmpxchg_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw16.cmpxchg_u", 2 },
    { thread | Opcode::i64__atomic__rmw32__cmpxchg_u, ParameterEncoding::memory, SignatureCode::i64__i32_i64_i64, "i64.atomic.rmw32.cmpxchg_u", 4 },
};

void Opcode::buildMap()
{
    uint32_t count = 0;

    for (const auto& entry : info) {
        map.emplace_back(entry.name, count++);
    }

    std::sort(map.begin(), map.end(),
             [](const Entry& x, const Entry& y) { return x.name < y.name; });
}

std::optional<Opcode> Opcode::fromString(std::string_view name)
{
    if (map.empty()) {
        buildMap();
    }

    auto it = std::lower_bound(map.begin(), map.end(), Entry{name, 0},
            [](const Entry& x, const Entry& y) { return x.name < y.name; });

    if (it != std::end(map) && it->name == name) {
        return Opcode(info[it->index].opcode);
    }

    return {};
}

Opcode::Info* Opcode::getInfo() const
{
    auto it = std::lower_bound(std::begin(info), std::end(info), Info{value},
            [](const Info& x, const Info& y) { return x.opcode < y.opcode; });

    if (it != std::end(info) && it->opcode == value) {
        return &*it;
    }

    return nullptr;
}

std::string_view Opcode::getName() const
{
    if (auto* info = getInfo(); info == nullptr) {
        return "<unknown>";
    } else {
        return info->name;
    }
}

bool Opcode::isValid() const
{
    return getInfo() != nullptr;
}

ParameterEncoding Opcode::getParameterEncoding() const
{
    if (auto* info = getInfo(); info == nullptr) {
        return ParameterEncoding::none;
    } else {
        return info->encoding;
    }
}

uint32_t Opcode::getAlign() const
{
    if (auto* info = getInfo(); info == nullptr) {
        return 0;
    } else {
        return info->align;
    }

    return info[value].align;
}

bool SectionType::isValid() const
{
    return value <= max;
}

std::string_view SectionType::getName() const
{
    switch (value) {
        case SectionType::custom:       return "Custom";
        case SectionType::type:         return "Type";
        case SectionType::import:       return "Import";
        case SectionType::function:     return "Function";
        case SectionType::table:        return "Table";
        case SectionType::memory:       return "Memory";
        case SectionType::global:       return "Global";
        case SectionType::export_:      return "Export";
        case SectionType::start:        return "Start";
        case SectionType::element:      return "Element";
        case SectionType::code:         return "Code";
        case SectionType::data:         return "Data";
        case SectionType::dataCount:    return "DataCount";
    }

    return std::string_view();
}

bool ValueType::isValid() const
{
    switch (value) {
        case i32:
        case i64:
        case f32:
        case f64:
        case v128:
        case anyref:
        case exnref:
        case func:
        case funcref:
        case void_:
            return true;
    }

    return false;
}

std::string_view ValueType::getName() const
{
    switch (value) {
        case i32:          return "i32";
        case i64:          return "i64";
        case f32:          return "f32";
        case f64:          return "f64";
        case v128:         return "v128";
        case anyref:       return "anyref";
        case exnref:       return "exnref";
        case func:         return "func";
        case void_:        return "void";
    }

    return std::string_view();
}

struct ValueTypeEntry
{
    std::string_view name;
    ValueType encoding;
};

ValueTypeEntry valueTypeMap[] = {
    { "anyref", ValueType::anyref },
    { "exnref", ValueType::exnref },
    { "f32", ValueType::f32 },
    { "f64", ValueType:: f64},
    { "func", ValueType::func },
    { "funcref", ValueType::funcref },
    { "i32", ValueType::i32 },
    { "i64", ValueType::i64 },
    { "v128", ValueType::v128 },
    { "void", ValueType::void_ }
};

std::optional<ValueType> ValueType::getEncoding(std::string_view n)
{
    if (auto it = std::lower_bound(std::begin(valueTypeMap), std::end(valueTypeMap),
                ValueTypeEntry{n, ValueType::void_},
                [](const ValueTypeEntry& x, const ValueTypeEntry& y) {
                return x.name < y.name; });
            it != std::end(valueTypeMap) && it->name == n) {
        return it->encoding;
    }

    return {};
}

std::string_view EventType::getName() const
{
    switch (value) {
        case exception: return "exception";
    }

    return std::string_view();
}

std::string_view ExternalType::getName() const
{
    switch (value) {
        case function:  return "func";
        case table:     return "table";
        case memory:    return "memory";
        case global:    return "global";
        case event:      return "event";
    }

    return std::string_view();
}

bool ExternalType::isValid() const
{
    switch (value) {
        case function:
        case table:
        case memory:
        case global:
        case event:
            return true;
    }

    return false;
}

std::optional<ExternalType> ExternalType::getEncoding(std::string_view name)
{
    if (name == "func") {
        return function;
    }

    if (name == "table") {
        return table;
    }

    if (name == "memory") {
        return memory;
    }

    if (name == "global") {
        return global;
    }

    if (name == "event") {
        return event;
    }

    return {};
}

std::ostream& operator<<(std::ostream& os, RelocationType type)
{
    const char* name = "<unknown>";

    switch (type) {
        case RelocationType::functionIndexLeb:     name = "R_WASM_FUNCTION_INDEX_LEB"; break;
        case RelocationType::tableIndexSleb:       name = "R_WASM_TABLE_INDEX_SLEB"; break;
        case RelocationType::tableIndexI32:        name = "R_WASM_TABLE_INDEX_I32"; break;
        case RelocationType::memoryAddrLeb:        name = "R_WASM_MEMORY_ADDR_LEB"; break;
        case RelocationType::memoryAddrSleb:       name = "R_WASM_MEMORY_ADDR_SLEB"; break;
        case RelocationType::memoryAddrI32:        name = "R_WASM_MEMORY_ADDR_I32"; break;
        case RelocationType::typeIndexLeb:         name = "R_WASM_TYPE_INDEX_LEB"; break;
        case RelocationType::globaLIndexLeb:       name = "R_WASM_GLOBAL_INDEX_LEB"; break;
        case RelocationType::functionOffsetI32:    name = "R_WASM_FUNCTION_OFFSET_I32"; break;
        case RelocationType::sectionOffsetI32:     name = "R_WASM_SECTION_OFFSET_I32"; break;
        case RelocationType::eventIndexLeb:        name = "R_WASM_EVENT_INDEX_LEB"; break;
        case RelocationType::memoryAddrRelSleb:    name = "R_WASM_MEMORY_ADDR_REL_SLEB"; break;
        case RelocationType::tableIndexRelSleb:    name = "R_WASM_TABLE_INDEX_REL_SLEB"; break;
    }

    os << name;
    return os;
}

std::ostream& operator<<(std::ostream& os, LinkingType type)
{
    const char* name = "<unknown>";

    switch (type) {
        case LinkingType::segmentInfo:  name ="WASM_SEGMENT_INFO"; break;
        case LinkingType::initFuncs:    name ="WASM_INIT_FUNCS"; break;
        case LinkingType::comDatInfo:   name ="WASM_COMDAT_INFO"; break;
        case LinkingType::symbolTable:  name ="WASM_SYMBOL_TABLE"; break;
    }

    os << name;
    return os;
}

std::ostream& operator<<(std::ostream& os, ComdatSymKind type)
{
    const char* name = "<unknown>";

    switch (type) {
        case ComdatSymKind::data:       name ="WASM_COMDAT_DATA"; break;
        case ComdatSymKind::function:   name ="WASM_COMDAT_FUNCTION"; break;
        case ComdatSymKind::global:     name ="WASM_COMDAT_GLOBAL"; break;
        case ComdatSymKind::event:      name ="WASM_COMDAT_EVENT"; break;
        case ComdatSymKind::table:      name ="WASM_COMDAT_TABLE"; break;
    }

    os << name;
    return os;
}

std::ostream& operator<<(std::ostream& os, SymbolKind type)
{
    const char* name = "<unknown>";

    switch (type) {
        case SymbolKind::function:  name ="SYMTAB_FUNCTION"; break;
        case SymbolKind::data:      name ="SYMTAB_DATA"; break;
        case SymbolKind::global:    name ="SYMTAB_GLOBAL"; break;
        case SymbolKind::section:   name ="SYMTAB_SECTION"; break;
        case SymbolKind::event:     name ="SYMTAB_EVENT"; break;
        case SymbolKind::table:     name ="SYMTAB_TABLE"; break;
    }

    os << name;
    return os;
}

bool hasAddend(RelocationType type)
{
    return type == RelocationType::memoryAddrLeb ||
      type == RelocationType::memoryAddrSleb ||
      type == RelocationType::memoryAddrI32 ||
      type == RelocationType::memoryAddrRelSleb ||
      type == RelocationType::functionOffsetI32 ||
      type == RelocationType::sectionOffsetI32;
}

