// Encodings.cpp

#include "Encodings.h"

#include <algorithm>

namespace libwasm
{

Opcode::Info Opcode::info[] =
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
    { Opcode::br_table, ImmediateType::table, SignatureCode::special, "br_table", 0 },
    { Opcode::return_, ImmediateType::none, SignatureCode::special, "return", 0 },
    { Opcode::call, ImmediateType::functionIdx, SignatureCode::special, "call", 0 },
    { Opcode::call_indirect, ImmediateType::indirect, SignatureCode::special, "call_indirect", 0 },
    { Opcode::return_call, ImmediateType::functionIdx, SignatureCode::special, "return_call", 0 },
    { Opcode::return_call_indirect, ImmediateType::functionIdx, SignatureCode::special, "return_call_indirect", 0 },
    { Opcode::drop, ImmediateType::none, SignatureCode::special, "drop", 0 },
    { Opcode::select, ImmediateType::none, SignatureCode::special, "select", 0 },
    { Opcode::local__get, ImmediateType::localIdx, SignatureCode::special, "local.get", 0 },
    { Opcode::local__set, ImmediateType::localIdx, SignatureCode::special, "local.set", 0 },
    { Opcode::local__tee, ImmediateType::localIdx, SignatureCode::special, "local.tee", 0 },
    { Opcode::global__get, ImmediateType::globalIdx, SignatureCode::special, "global.get", 0 },
    { Opcode::global__set, ImmediateType::globalIdx, SignatureCode::special, "global.set", 0 },
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
    { Opcode::alloca, ImmediateType::none, SignatureCode::void_, "alloca", 0 },
    { Opcode::br_unless, ImmediateType::none, SignatureCode::special, "br_unless", 0 },
    { Opcode::call_host, ImmediateType::none, SignatureCode::void_, "call_host", 0 },
    { Opcode::data, ImmediateType::none, SignatureCode::void_, "data", 0 },
    { Opcode::drop_keep, ImmediateType::none, SignatureCode::void_, "drop_keep", 0 },
    { Opcode::ref__null, ImmediateType::none, SignatureCode::void_, "ref.null", 0 },
    { Opcode::ref__is_null, ImmediateType::none, SignatureCode::void_, "ref.is_null", 0 },
    { Opcode::ref__func, ImmediateType::functionIdx, SignatureCode::void_, "ref.func", 0 },

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
    { Opcode::table__init, ImmediateType::elementIdxTable, SignatureCode::void__i32_i32_i32, "table.init", 0 },
    { Opcode::elem__drop, ImmediateType::elementIdx, SignatureCode::void_, "elem.drop", 0 },
    { Opcode::table__copy, ImmediateType::tableTable, SignatureCode::void__i32_i32_i32, "table.copy", 0 },
    { Opcode::table__get, ImmediateType::none, SignatureCode::void_, "table.get", 0 },
    { Opcode::table__set, ImmediateType::none, SignatureCode::void_, "table.set", 0 },
    { Opcode::table__grow, ImmediateType::none, SignatureCode::void_, "table.grow", 0 },
    { Opcode::table__size, ImmediateType::none, SignatureCode::void_, "table.size", 0 },
    { Opcode::table__fill, ImmediateType::none, SignatureCode::void_, "table.fill", 0 },

    // SIMD
    { Opcode::v128__load, ImmediateType::memory, SignatureCode::v128__i32, "v128.load", 16 },
    { Opcode::v128__store, ImmediateType::memory, SignatureCode::void__i32_v128, "v128.store", 16 },
    { Opcode::v128__const, ImmediateType::v128, SignatureCode::v128_, "v128.const", 0 },
    { Opcode::i8x16__splat, ImmediateType::none, SignatureCode::v128__i32, "i8x16.splat", 0 },
    { Opcode::i8x16__extract_lane_s, ImmediateType::lane16Idx, SignatureCode::i32__v128, "i8x16.extract_lane_s", 0 },
    { Opcode::i8x16__extract_lane_u, ImmediateType::lane16Idx, SignatureCode::i32__v128, "i8x16.extract_lane_u", 0 },
    { Opcode::i8x16__replace_lane, ImmediateType::lane16Idx, SignatureCode::v128__v128_i32, "i8x16.replace_lane", 0 },
    { Opcode::i16x8__splat, ImmediateType::none, SignatureCode::v128__i32, "i16x8.splat", 0 },
    { Opcode::i16x8__extract_lane_s, ImmediateType::lane8Idx, SignatureCode::i32__v128, "i16x8.extract_lane_s", 0 },
    { Opcode::i16x8__extract_lane_u, ImmediateType::lane8Idx, SignatureCode::i32__v128, "i16x8.extract_lane_u", 0 },
    { Opcode::i16x8__replace_lane, ImmediateType::lane8Idx, SignatureCode::v128__v128_i32, "i16x8.replace_lane", 0 },
    { Opcode::i32x4__splat, ImmediateType::none, SignatureCode::v128__i32, "i32x4.splat", 0 },
    { Opcode::i32x4__extract_lane, ImmediateType::lane4Idx, SignatureCode::i32__v128, "i32x4.extract_lane", 0 },
    { Opcode::i32x4__replace_lane, ImmediateType::lane4Idx, SignatureCode::v128__v128_i32, "i32x4.replace_lane", 0 },
    { Opcode::i64x2__splat, ImmediateType::none, SignatureCode::v128__i64, "i64x2.splat", 0 },
    { Opcode::i64x2__extract_lane, ImmediateType::lane2Idx, SignatureCode::i64__v128, "i64x2.extract_lane", 0 },
    { Opcode::i64x2__replace_lane, ImmediateType::lane2Idx, SignatureCode::v128__v128_i64, "i64x2.replace_lane", 0 },
    { Opcode::f32x4__splat, ImmediateType::none, SignatureCode::v128__f32, "f32x4.splat", 0 },
    { Opcode::f32x4__extract_lane, ImmediateType::lane4Idx, SignatureCode::f32__v128, "f32x4.extract_lane", 0 },
    { Opcode::f32x4__replace_lane, ImmediateType::lane4Idx, SignatureCode::v128__v128_f32, "f32x4.replace_lane", 0 },
    { Opcode::f64x2__splat, ImmediateType::none, SignatureCode::v128__f64, "f64x2.splat", 0 },
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
    { Opcode::v128__or, ImmediateType::none, SignatureCode::v128__v128_v128, "v128.or", 0 },
    { Opcode::v128__xor, ImmediateType::none, SignatureCode::v128__v128_v128, "v128.xor", 0 },
    { Opcode::v128__bitselect, ImmediateType::none, SignatureCode::v128__v128_v128_v128, "v128.bitselect", 0 },
    { Opcode::i8x16__neg, ImmediateType::none, SignatureCode::v128__v128, "i8x16.neg", 0 },
    { Opcode::i8x16__any_true, ImmediateType::none, SignatureCode::i32__v128, "i8x16.any_true", 0 },
    { Opcode::i8x16__all_true, ImmediateType::none, SignatureCode::i32__v128, "i8x16.all_true", 0 },
    { Opcode::i8x16__shl, ImmediateType::none, SignatureCode::v128__v128_i32, "i8x16.shl", 0 },
    { Opcode::i8x16__shr_s, ImmediateType::none, SignatureCode::v128__v128_i32, "i8x16.shr_s", 0 },
    { Opcode::i8x16__shr_u, ImmediateType::none, SignatureCode::v128__v128_i32, "i8x16.shr_u", 0 },
    { Opcode::i8x16__add, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.add", 0 },
    { Opcode::i8x16__add_saturate_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.add_saturate_s", 0 },
    { Opcode::i8x16__add_saturate_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.add_saturate_u", 0 },
    { Opcode::i8x16__sub, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.sub", 0 },
    { Opcode::i8x16__sub_saturate_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.sub_saturate_s", 0 },
    { Opcode::i8x16__sub_saturate_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.sub_saturate_u", 0 },
    { Opcode::i8x16__mul, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.mul", 0 },
    { Opcode::i8x16__min_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.min_s", 0 },
    { Opcode::i8x16__min_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.min_u", 0 },
    { Opcode::i8x16__max_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.max_s", 0 },
    { Opcode::i8x16__max_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.max_u", 0 },
    { Opcode::i16x8__neg, ImmediateType::none, SignatureCode::v128__v128, "i16x8.neg", 0 },
    { Opcode::i16x8__any_true, ImmediateType::none, SignatureCode::i32__v128, "i16x8.any_true", 0 },
    { Opcode::i16x8__all_true, ImmediateType::none, SignatureCode::i32__v128, "i16x8.all_true", 0 },
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
    { Opcode::i32x4__neg, ImmediateType::none, SignatureCode::v128__v128, "i32x4.neg", 0 },
    { Opcode::i32x4__any_true, ImmediateType::none, SignatureCode::i32__v128, "i32x4.any_true", 0 },
    { Opcode::i32x4__all_true, ImmediateType::none, SignatureCode::i32__v128, "i32x4.all_true", 0 },
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
    { Opcode::i64x2__any_true, ImmediateType::none, SignatureCode::i32__v128, "i64x2.any_true", 0 },
    { Opcode::i64x2__all_true, ImmediateType::none, SignatureCode::i32__v128, "i64x2.all_true", 0 },
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
    { Opcode::i64x2__trunc_sat_f64x2_s, ImmediateType::none, SignatureCode::v128__v128, "i64x2.trunc_sat_f64x2_s", 0 },
    { Opcode::i64x2__trunc_sat_f64x2_u, ImmediateType::none, SignatureCode::v128__v128, "i64x2.trunc_sat_f64x2_u", 0 },
    { Opcode::f32x4__convert_i32x4_s, ImmediateType::none, SignatureCode::v128__v128, "f32x4.convert_i32x4_s", 0 },
    { Opcode::f32x4__convert_i32x4_u, ImmediateType::none, SignatureCode::v128__v128, "f32x4.convert_i32x4_u", 0 },
    { Opcode::f64x2__convert_i64x2_s, ImmediateType::none, SignatureCode::v128__v128, "f64x2.convert_i64x2_s", 0 },
    { Opcode::f64x2__convert_i64x2_u, ImmediateType::none, SignatureCode::v128__v128, "f64x2.convert_i64x2_u", 0 },
    { Opcode::v8x16__swizzle, ImmediateType::none, SignatureCode::v128__v128_v128, "v8x16.swizzle", 0 },
    { Opcode::v8x16__shuffle, ImmediateType::shuffle, SignatureCode::v128__v128_v128, "v8x16.shuffle", 0 },
    { Opcode::v8x16__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v8x16.load_splat", 1 },
    { Opcode::v16x8__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v16x8.load_splat", 2 },
    { Opcode::v32x4__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v32x4.load_splat", 4 },
    { Opcode::v64x2__load_splat, ImmediateType::memory, SignatureCode::v128__i32, "v64x2.load_splat", 8 },
    { Opcode::i8x16__narrow_i16x8_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.narrow_i16x8_s", 0 },
    { Opcode::i8x16__narrow_i16x8_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.narrow_i16x8_u", 0 },
    { Opcode::i16x8__narrow_i32x4_s, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.narrow_i32x4_s", 0 },
    { Opcode::i16x8__narrow_i32x4_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.narrow_i32x4_u", 0 },
    { Opcode::i16x8__widen_low_i8x16_s, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_low_i8x16_s", 0 },
    { Opcode::i16x8__widen_high_i8x16_s, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_high_i8x16_s", 0 },
    { Opcode::i16x8__widen_low_i8x16_u, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_low_i8x16_u", 0 },
    { Opcode::i16x8__widen_high_i8x16_u, ImmediateType::none, SignatureCode::v128__v128, "i16x8.widen_high_i8x16_u", 0 },
    { Opcode::i32x4__widen_low_i16x8_s, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_low_i16x8_s", 0 },
    { Opcode::i32x4__widen_high_i16x8_s, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_high_i16x8_s", 0 },
    { Opcode::i32x4__widen_low_i16x8_u, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_low_i16x8_u", 0 },
    { Opcode::i32x4__widen_high_i16x8_u, ImmediateType::none, SignatureCode::v128__v128, "i32x4.widen_high_i16x8_u", 0 },
    { Opcode::i16x8__load8x8_s, ImmediateType::memory, SignatureCode::v128__i32, "i16x8.load8x8_s", 8 },
    { Opcode::i16x8__load8x8_u, ImmediateType::memory, SignatureCode::v128__i32, "i16x8.load8x8_u", 8 },
    { Opcode::i32x4__load16x4_s, ImmediateType::memory, SignatureCode::v128__i32, "i32x4.load16x4_s", 8 },
    { Opcode::i32x4__load16x4_u, ImmediateType::memory, SignatureCode::v128__i32, "i32x4.load16x4_u", 8 },
    { Opcode::i64x2__load32x2_s, ImmediateType::memory, SignatureCode::v128__i32, "i64x2.load32x2_s", 8 },
    { Opcode::i64x2__load32x2_u, ImmediateType::memory, SignatureCode::v128__i32, "i64x2.load32x2_u", 8 },
    { Opcode::v128__andnot, ImmediateType::none, SignatureCode::v128__v128_v128, "v128.andnot", 0 },
    { Opcode::i8x16__avgr_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i8x16.avgr_u", 0 },
    { Opcode::i16x8__avgr_u, ImmediateType::none, SignatureCode::v128__v128_v128, "i16x8.avgr_u", 0 },
    { Opcode::i8x16__abs, ImmediateType::none, SignatureCode::v128__v128, "i8x16.abs", 0 },
    { Opcode::i16x8__abs, ImmediateType::none, SignatureCode::v128__v128, "i16x8.abs", 0 },
    { Opcode::i32x4__abs, ImmediateType::none, SignatureCode::v128__v128, "i32x4.abs", 0 },

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

std::vector<Opcode::Entry> Opcode::map;

Opcode::Initializer Opcode::initializer;

void Opcode::buildMap()
{
    std::sort(std::begin(info), std::end(info),
            [](const Info& x, const Info& y) { return x.opcode < y.opcode; });

    uint32_t count = 0;

    map.reserve(std::size(info));

    for (const auto& entry : info) {
        map.emplace_back(entry.name, count++);
    }

    std::sort(map.begin(), map.end(),
             [](const Entry& x, const Entry& y) { return x.name < y.name; });
}

std::optional<Opcode> Opcode::fromString(std::string_view name)
{
    auto it = std::lower_bound(map.begin(), map.end(), Entry{name, 0},
            [](const Entry& x, const Entry& y) { return x.name < y.name; });

    if (it != std::end(map) && it->name == name) {
        return Opcode(info[it->index].opcode);
    }

    return {};
}

const Opcode::Info* Opcode::getInfo() const
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

ImmediateType Opcode::getImmediateType() const
{
    if (auto* info = getInfo(); info == nullptr) {
        return ImmediateType::none;
    } else {
        return info->type;
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
        case SectionType::event:        return "Event";
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
        case funcref:
        case nullref:
        case void_:
            return true;
    }

    return false;
}

bool ValueType::isValidNumeric() const
{
    switch (value) {
        case i32:
        case i64:
        case f32:
        case f64:
        case v128:
            return true;

        default:
            break;
    }

    return false;
}

bool ValueType::isValidRef() const
{
    switch (value) {
        case anyref:
        case exnref:
        case funcref:
        case nullref:
            return true;

        default:
            break;
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
        case nullref:      return "nullref";
        case funcref:      return "funcref";
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

};
