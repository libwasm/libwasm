// Encodings.cpp

#include "Encodings.h"

#include <algorithm>

Opcode::Entry Opcode::map[] =
{
    { "block", 0x02 },
    { "br", 0x0c },
    { "br_if", 0x0d },
    { "br_table", 0x0e },
    { "call", 0x10 },
    { "call_indirect", 0x11 },
    { "drop", 0x1a },
    { "else", 0x05 },
    { "end", 0x0b },
    { "f32.abs", 0x8b },
    { "f32.add", 0x92 },
    { "f32.ceil", 0x8d },
    { "f32.const", 0x43 },
    { "f32.convert_i32_s", 0xb2 },
    { "f32.convert_i32_u", 0xb3 },
    { "f32.convert_i64_s", 0xb4 },
    { "f32.convert_i64_u", 0xb5 },
    { "f32.copysign", 0x98 },
    { "f32.demote_f64", 0xb6 },
    { "f32.div", 0x95 },
    { "f32.eq", 0x5b },
    { "f32.floor", 0x8e },
    { "f32.ge", 0x60 },
    { "f32.gt", 0x5e },
    { "f32.le", 0x5f },
    { "f32.load", 0x2a },
    { "f32.lt", 0x5d },
    { "f32.max", 0x97 },
    { "f32.min", 0x96 },
    { "f32.mul", 0x94 },
    { "f32.ne", 0x5c },
    { "f32.nearest", 0x90 },
    { "f32.neg", 0x8c },
    { "f32.reinterpret_i32", 0xbe },
    { "f32.sqrt", 0x91 },
    { "f32.store", 0x38 },
    { "f32.sub", 0x93 },
    { "f32.trunc", 0x8f },
    { "f64.abs", 0x99 },
    { "f64.add", 0xa0 },
    { "f64.ceil", 0x9b },
    { "f64.const", 0x44 },
    { "f64.convert_i32_s", 0xb7 },
    { "f64.convert_i32_u", 0xb8 },
    { "f64.convert_i64_s", 0xb9 },
    { "f64.convert_i64_u", 0xba },
    { "f64.copysign", 0xa6 },
    { "f64.div", 0xa3 },
    { "f64.eq", 0x61 },
    { "f64.floor", 0x9c },
    { "f64.ge", 0x66 },
    { "f64.gt", 0x64 },
    { "f64.le", 0x65 },
    { "f64.load", 0x2b },
    { "f64.lt", 0x63 },
    { "f64.max", 0xa5 },
    { "f64.min", 0xa4 },
    { "f64.mul", 0xa2 },
    { "f64.ne", 0x62 },
    { "f64.nearest", 0x9e },
    { "f64.neg", 0x9a },
    { "f64.promote_f32", 0xbb },
    { "f64.reinterpret_i64", 0xbf },
    { "f64.sqrt", 0x9f },
    { "f64.store", 0x39 },
    { "f64.sub", 0xa1 },
    { "f64.trunc", 0x9d },
    { "global.get", 0x23 },
    { "global.set", 0x24 },
    { "i32.add", 0x6a },
    { "i32.and", 0x71 },
    { "i32.clz", 0x67 },
    { "i32.const", 0x41 },
    { "i32.ctz", 0x68 },
    { "i32.div_s", 0x6d },
    { "i32.div_u", 0x6e },
    { "i32.eq", 0x46 },
    { "i32.eqz", 0x45 },
    { "i32.ge_s", 0x4e },
    { "i32.ge_u", 0x4f },
    { "i32.gt_s", 0x4a },
    { "i32.gt_u", 0x4b },
    { "i32.le_s", 0x4c },
    { "i32.le_u", 0x4d },
    { "i32.load", 0x28 },
    { "i32.load16_s", 0x2e },
    { "i32.load16_u", 0x2f },
    { "i32.load8_s", 0x2c },
    { "i32.load8_u", 0x2d },
    { "i32.lt_s", 0x48 },
    { "i32.lt_u", 0x49 },
    { "i32.mul", 0x6c },
    { "i32.ne", 0x47 },
    { "i32.or", 0x72 },
    { "i32.popcnt", 0x69 },
    { "i32.reinterpret_f32", 0xbc },
    { "i32.rem_s", 0x6f },
    { "i32.rem_u", 0x70 },
    { "i32.rotl", 0x77 },
    { "i32.rotr", 0x78 },
    { "i32.shl", 0x74 },
    { "i32.shr_s", 0x75 },
    { "i32.shr_u", 0x76 },
    { "i32.store", 0x36 },
    { "i32.store16", 0x3b },
    { "i32.store8", 0x3a },
    { "i32.sub", 0x6b },
    { "i32.trunc_f32_s", 0xa8 },
    { "i32.trunc_f32_u", 0xa9 },
    { "i32.trunc_f64_s", 0xaa },
    { "i32.trunc_f64_u", 0xab },
    { "i32.wrap_i64", 0xa7 },
    { "i32.xor", 0x73 },
    { "i64.", 0x54 },
    { "i64.add", 0x7c },
    { "i64.and", 0x83 },
    { "i64.clz", 0x79 },
    { "i64.const", 0x42 },
    { "i64.ctz", 0x7a },
    { "i64.div_s", 0x7f },
    { "i64.div_u", 0x80 },
    { "i64.eq", 0x51 },
    { "i64.eqz", 0x50 },
    { "i64.extend_i32_s", 0xac },
    { "i64.extend_i32_u", 0xad },
    { "i64.ge_s", 0x59 },
    { "i64.ge_u", 0x5a },
    { "i64.gt_s", 0x55 },
    { "i64.gt_u", 0x56 },
    { "i64.le_s", 0x57 },
    { "i64.le_u", 0x58 },
    { "i64.load", 0x29 },
    { "i64.load16_s", 0x32 },
    { "i64.load16_u", 0x33 },
    { "i64.load32_s", 0x34 },
    { "i64.load32_u", 0x35 },
    { "i64.load8_s", 0x30 },
    { "i64.load8_u", 0x31 },
    { "i64.lt_s", 0x53 },
    { "i64.mul", 0x7e },
    { "i64.ne", 0x52 },
    { "i64.or", 0x84 },
    { "i64.popcnt", 0x7b },
    { "i64.reinterpret_f64", 0xbd },
    { "i64.rem_s", 0x81 },
    { "i64.rem_u", 0x82 },
    { "i64.rotl", 0x89 },
    { "i64.rotr", 0x8a },
    { "i64.shl", 0x86 },
    { "i64.shr_s", 0x87 },
    { "i64.shr_u", 0x88 },
    { "i64.store", 0x37 },
    { "i64.store16", 0x3d },
    { "i64.store32", 0x3e },
    { "i64.store8", 0x3c },
    { "i64.sub", 0x7d },
    { "i64.trunc_f32_s", 0xae },
    { "i64.trunc_f32_u", 0xaf },
    { "i64.trunc_f64_s", 0xb0 },
    { "i64.trunc_f64_u", 0xb1 },
    { "i64.xor", 0x85 },
    { "if", 0x04 },
    { "local.get", 0x20 },
    { "local.set", 0x21 },
    { "local.tee", 0x22 },
    { "loop", 0x03 },
    { "memory.grow", 0x40 },
    { "memory.size", 0x3f },
    { "nop", 0x01 },
    { "return", 0x0f },
    { "select", 0x1b },
    { "unreachable", 0x00 }
};

Opcode::Info Opcode::info[size_t(Opcode::max) + 1] =
{
    { ParameterEncoding::none, "unreachable" },                      // 0x00
    { ParameterEncoding::none, "nop" },                              // 0x01
    { ParameterEncoding::block, "block" },                           // 0x02
    { ParameterEncoding::block, "loop" },                            // 0x03
    { ParameterEncoding::block, "if" },                              // 0x04
    { ParameterEncoding::none, "else" },                             // 0x05
    { ParameterEncoding::none },                                     // 0x06
    { ParameterEncoding::none },                                     // 0x07
    { ParameterEncoding::none },                                     // 0x08
    { ParameterEncoding::none },                                     // 0x09
    { ParameterEncoding::none },                                     // 0x0A
    { ParameterEncoding::none, "end" },                              // 0x0B
    { ParameterEncoding::labelIdx, "br" },                           // 0x0C
    { ParameterEncoding::labelIdx, "br_if" },                        // 0x0D
    { ParameterEncoding::table, "br_table" },                        // 0x0E
    { ParameterEncoding::none, "return" },                           // 0x0F

    { ParameterEncoding::functionIdx, "call" },                      // 0x10
    { ParameterEncoding::indirect, "call_indirect" },                // 0x11
    { ParameterEncoding::none },                                     // 0x12
    { ParameterEncoding::none },                                     // 0x13
    { ParameterEncoding::none },                                     // 0x14
    { ParameterEncoding::none },                                     // 0x15
    { ParameterEncoding::none },                                     // 0x16
    { ParameterEncoding::none },                                     // 0x17
    { ParameterEncoding::none },                                     // 0x18
    { ParameterEncoding::none },                                     // 0x19
    { ParameterEncoding::none, "drop" },                             // 0x1A
    { ParameterEncoding::none, "select" },                           // 0x1B
    { ParameterEncoding::none },                                     // 0x1C
    { ParameterEncoding::none },                                     // 0x1D
    { ParameterEncoding::none },                                     // 0x1E
    { ParameterEncoding::none },                                     // 0x1F

    { ParameterEncoding::localIdx, "local.get" },                    // 0x20
    { ParameterEncoding::localIdx, "local.set" },                    // 0x21
    { ParameterEncoding::localIdx, "local.tee" },                    // 0x22
    { ParameterEncoding::globalIdx, "global.get" },                  // 0x23
    { ParameterEncoding::globalIdx, "global.set" },                  // 0x24
    { ParameterEncoding::none },                                     // 0x25
    { ParameterEncoding::none },                                     // 0x26
    { ParameterEncoding::none },                                     // 0x27
    { ParameterEncoding::memory, "i32.load", 2 },                    // 0x28
    { ParameterEncoding::memory, "i64.load", 3 },                    // 0x29
    { ParameterEncoding::memory, "f32.load", 2, true },              // 0x2A
    { ParameterEncoding::memory, "f64.load", 3, true },              // 0x2B
    { ParameterEncoding::memory, "i32.load8_s", 0 },                 // 0x2C
    { ParameterEncoding::memory, "i32.load8_u", 0 },                 // 0x2D
    { ParameterEncoding::memory, "i32.load16_s", 1 },                // 0x2E
    { ParameterEncoding::memory, "i32.load16_u", 1 },                // 0x2F

    { ParameterEncoding::memory, "i64.load8_s", 0 },                 // 0x30
    { ParameterEncoding::memory, "i64.load8_u", 0 },                 // 0x31
    { ParameterEncoding::memory, "i64.load16_s", 1 },                // 0x32
    { ParameterEncoding::memory, "i64.load16_u", 1 },                // 0x33
    { ParameterEncoding::memory, "i64.load32_s", 2 },                // 0x34
    { ParameterEncoding::memory, "i64.load32_u", 2 },                // 0x35
    { ParameterEncoding::memory, "i32.store", 2 },                   // 0x36
    { ParameterEncoding::memory, "i64.store", 3 },                   // 0x37
    { ParameterEncoding::memory, "f32.store", 2, true },             // 0x38
    { ParameterEncoding::memory, "f64.store", 3, true },             // 0x39
    { ParameterEncoding::memory, "i32.store8", 0 },                  // 0x3A
    { ParameterEncoding::memory, "i32.store16", 1 },                 // 0x3B
    { ParameterEncoding::memory, "i64.store8", 0 },                  // 0x3C
    { ParameterEncoding::memory, "i64.store16", 1 },                 // 0x3D
    { ParameterEncoding::memory, "i64.store32", 2 },                 // 0x3E
    { ParameterEncoding::memory0, "memory.size" },                   // 0x3F

    { ParameterEncoding::memory0, "memory.grow" },                   // 0x40
    { ParameterEncoding::i32, "i32.const", 2 },                      // 0x41
    { ParameterEncoding::i64, "i64.const", 3 },                      // 0x42
    { ParameterEncoding::f32, "f32.const", 2, true },                // 0x43
    { ParameterEncoding::f64, "f64.const", 3, true },                // 0x44
    { ParameterEncoding::none, "i32.eqz", 2 },                       // 0x45
    { ParameterEncoding::none, "i32.eq", 2 },                        // 0x46
    { ParameterEncoding::none, "i32.ne", 2 },                        // 0x47
    { ParameterEncoding::none, "i32.lt_s", 2 },                      // 0x48
    { ParameterEncoding::none, "i32.lt_u", 2 },                      // 0x49
    { ParameterEncoding::none, "i32.gt_s", 2 },                      // 0x4A
    { ParameterEncoding::none, "i32.gt_u", 2 },                      // 0x4B
    { ParameterEncoding::none, "i32.le_s", 2 },                      // 0x4C
    { ParameterEncoding::none, "i32.le_u", 2 },                      // 0x4D
    { ParameterEncoding::none, "i32.ge_s", 2 },                      // 0x4E
    { ParameterEncoding::none, "i32.ge_u", 2 },                      // 0x4F

    { ParameterEncoding::none, "i64.eqz", 3 },                       // 0x50
    { ParameterEncoding::none, "i64.eq", 3 },                        // 0x51
    { ParameterEncoding::none, "i64.ne", 3 },                        // 0x52
    { ParameterEncoding::none, "i64.lt_s", 3 },                      // 0x53
    { ParameterEncoding::none, "i64.", 3 },                          // 0x54
    { ParameterEncoding::none, "i64.gt_s", 3 },                      // 0x55
    { ParameterEncoding::none, "i64.gt_u", 3 },                      // 0x56
    { ParameterEncoding::none, "i64.le_s", 3 },                      // 0x57
    { ParameterEncoding::none, "i64.le_u", 3 },                      // 0x58
    { ParameterEncoding::none, "i64.ge_s", 3 },                      // 0x59
    { ParameterEncoding::none, "i64.ge_u", 3 },                      // 0x5A
    { ParameterEncoding::none, "f32.eq", 2, true },                  // 0x5B
    { ParameterEncoding::none, "f32.ne", 2, true },                  // 0x5C
    { ParameterEncoding::none, "f32.lt", 2, true },                  // 0x5D
    { ParameterEncoding::none, "f32.gt", 2, true },                  // 0x5E
    { ParameterEncoding::none, "f32.le", 2, true },                  // 0x5F

    { ParameterEncoding::none, "f32.ge", 2, true },                  // 0x60
    { ParameterEncoding::none, "f64.eq", 3, true },                  // 0x61
    { ParameterEncoding::none, "f64.ne", 3, true },                  // 0x62
    { ParameterEncoding::none, "f64.lt", 3, true },                  // 0x63
    { ParameterEncoding::none, "f64.gt", 3, true },                  // 0x64
    { ParameterEncoding::none, "f64.le", 3, true },                  // 0x65
    { ParameterEncoding::none, "f64.ge", 3, true },                  // 0x66
    { ParameterEncoding::none, "i32.clz", 2 },                       // 0x67
    { ParameterEncoding::none, "i32.ctz", 2 },                       // 0x68
    { ParameterEncoding::none, "i32.popcnt", 2 },                    // 0x69
    { ParameterEncoding::none, "i32.add", 2 },                       // 0x6A
    { ParameterEncoding::none, "i32.sub", 2 },                       // 0x6B
    { ParameterEncoding::none, "i32.mul", 2 },                       // 0x6C
    { ParameterEncoding::none, "i32.div_s", 2 },                     // 0x6D
    { ParameterEncoding::none, "i32.div_u", 2 },                     // 0x6E
    { ParameterEncoding::none, "i32.rem_s", 2 },                     // 0x6F

    { ParameterEncoding::none, "i32.rem_u", 2 },                     // 0x70
    { ParameterEncoding::none, "i32.and", 2 },                       // 0x71
    { ParameterEncoding::none, "i32.or", 2 },                        // 0x72
    { ParameterEncoding::none, "i32.xor", 2 },                       // 0x73
    { ParameterEncoding::none, "i32.shl", 2 },                       // 0x74
    { ParameterEncoding::none, "i32.shr_s", 2 },                     // 0x75
    { ParameterEncoding::none, "i32.shr_u", 2 },                     // 0x76
    { ParameterEncoding::none, "i32.rotl", 2 },                      // 0x77
    { ParameterEncoding::none, "i32.rotr", 2 },                      // 0x78
    { ParameterEncoding::none, "i64.clz", 3 },                       // 0x79
    { ParameterEncoding::none, "i64.ctz", 3 },                       // 0x7A
    { ParameterEncoding::none, "i64.popcnt", 3 },                    // 0x7B
    { ParameterEncoding::none, "i64.add", 3 },                       // 0x7C
    { ParameterEncoding::none, "i64.sub", 3 },                       // 0x7D
    { ParameterEncoding::none, "i64.mul", 3 },                       // 0x7E
    { ParameterEncoding::none, "i64.div_s", 3 },                     // 0x7F

    { ParameterEncoding::none, "i64.div_u", 3 },                     // 0x80
    { ParameterEncoding::none, "i64.rem_s", 3 },                     // 0x81
    { ParameterEncoding::none, "i64.rem_u", 3 },                     // 0x82
    { ParameterEncoding::none, "i64.and", 3 },                       // 0x83
    { ParameterEncoding::none, "i64.or", 3 },                        // 0x84
    { ParameterEncoding::none, "i64.xor", 3 },                       // 0x85
    { ParameterEncoding::none, "i64.shl", 3 },                       // 0x86
    { ParameterEncoding::none, "i64.shr_s", 3 },                     // 0x87
    { ParameterEncoding::none, "i64.shr_u", 3 },                     // 0x88
    { ParameterEncoding::none, "i64.rotl", 3 },                      // 0x89
    { ParameterEncoding::none, "i64.rotr", 3 },                      // 0x8A
    { ParameterEncoding::none, "f32.abs", 2, true },                 // 0x8B
    { ParameterEncoding::none, "f32.neg", 2, true },                 // 0x8C
    { ParameterEncoding::none, "f32.ceil", 2, true },                // 0x8D
    { ParameterEncoding::none, "f32.floor", 2, true },               // 0x8E
    { ParameterEncoding::none, "f32.trunc", 2, true },               // 0x8F

    { ParameterEncoding::none, "f32.nearest", 2, true },             // 0x90
    { ParameterEncoding::none, "f32.sqrt", 2, true },                // 0x91
    { ParameterEncoding::none, "f32.add", 2, true },                 // 0x92
    { ParameterEncoding::none, "f32.sub", 2, true },                 // 0x93
    { ParameterEncoding::none, "f32.mul", 2, true },                 // 0x94
    { ParameterEncoding::none, "f32.div", 2, true },                 // 0x95
    { ParameterEncoding::none, "f32.min", 2, true },                 // 0x96
    { ParameterEncoding::none, "f32.max", 2, true },                 // 0x97
    { ParameterEncoding::none, "f32.copysign", 2, true },            // 0x98
    { ParameterEncoding::none, "f64.abs", 3, true },                 // 0x99
    { ParameterEncoding::none, "f64.neg", 3, true },                 // 0x9A
    { ParameterEncoding::none, "f64.ceil", 3, true },                // 0x9B
    { ParameterEncoding::none, "f64.floor", 3, true },               // 0x9C
    { ParameterEncoding::none, "f64.trunc", 3, true },               // 0x9D
    { ParameterEncoding::none, "f64.nearest", 3, true },             // 0x9E
    { ParameterEncoding::none, "f64.sqrt", 3, true },                // 0x9F

    { ParameterEncoding::none, "f64.add", 3, true },                 // 0xA0
    { ParameterEncoding::none, "f64.sub", 3, true },                 // 0xA1
    { ParameterEncoding::none, "f64.mul", 3, true },                 // 0xA2
    { ParameterEncoding::none, "f64.div", 3, true },                 // 0xA3
    { ParameterEncoding::none, "f64.min", 3, true },                 // 0xA4
    { ParameterEncoding::none, "f64.max", 3, true },                 // 0xA5
    { ParameterEncoding::none, "f64.copysign", 3, true },            // 0xA6
    { ParameterEncoding::none, "i32.wrap_i64", 2 },                  // 0xA7
    { ParameterEncoding::none, "i32.trunc_f32_s", 2 },               // 0xA8
    { ParameterEncoding::none, "i32.trunc_f32_u", 2 },               // 0xA9
    { ParameterEncoding::none, "i32.trunc_f64_s", 2 },               // 0xAA
    { ParameterEncoding::none, "i32.trunc_f64_u", 2 },               // 0xAB
    { ParameterEncoding::none, "i64.extend_i32_s", 3 },              // 0xAC
    { ParameterEncoding::none, "i64.extend_i32_u", 3 },              // 0xAD
    { ParameterEncoding::none, "i64.trunc_f32_s", 3 },               // 0xAE
    { ParameterEncoding::none, "i64.trunc_f32_u", 3 },               // 0xAF

    { ParameterEncoding::none, "i64.trunc_f64_s", 3 },               // 0xB0
    { ParameterEncoding::none, "i64.trunc_f64_u", 3 },               // 0xB1
    { ParameterEncoding::none, "f32.convert_i32_s", 2, true },       // 0xB2
    { ParameterEncoding::none, "f32.convert_i32_u", 2, true },       // 0xB3
    { ParameterEncoding::none, "f32.convert_i64_s", 2, true },       // 0xB4
    { ParameterEncoding::none, "f32.convert_i64_u", 2, true },       // 0xB5
    { ParameterEncoding::none, "f32.demote_f64", 2, true },          // 0xB6
    { ParameterEncoding::none, "f64.convert_i32_s", 3, true },       // 0xB7
    { ParameterEncoding::none, "f64.convert_i32_u", 3, true },       // 0xB8
    { ParameterEncoding::none, "f64.convert_i64_s", 3, true },       // 0xB9
    { ParameterEncoding::none, "f64.convert_i64_u", 3, true },       // 0xBA
    { ParameterEncoding::none, "f64.promote_f32", 3, true },         // 0xBB
    { ParameterEncoding::none, "i32.reinterpret_f32", 2 },           // 0xBC
    { ParameterEncoding::none, "i64.reinterpret_f64", 3 },           // 0xBD
    { ParameterEncoding::none, "f32.reinterpret_i32", 2, true },     // 0xBE
    { ParameterEncoding::none, "f64.reinterpret_i64", 3, true },     // 0xBF
};

std::optional<Opcode> Opcode::fromString(std::string_view name)
{
    auto it = std::lower_bound(std::begin(map), std::end(map), Entry{name, 0},
            [](const Entry& x, const Entry& y) { return x.name < y.name; });

    if (it != std::end(map) && it->name == name) {
        return Opcode(it->opcode);
    }

    return {};
}

std::string_view Opcode::getName() const
{
    if (!isValid()) {
        return "<unknown>";
    }

    return info[uint8_t(value)].name;
}

bool Opcode::isValid() const
{
    return (value <= max && !info[value].name.empty());
}

ParameterEncoding Opcode::getParameterEncoding() const
{
    if (!isValid()) {
        return ParameterEncoding::none;
    }

    return info[value].encoding;
}

uint32_t Opcode::getAlign() const
{
    if (!isValid()) {
        return 0;
    }

    return info[value].align;
}

bool Opcode::isfloat() const
{
    if (!isValid()) {
        return false;
    }

    return info[value].mFloat;
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
        case funcref:
        case anyref:
        case exnref:
        case func:
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
        case funcref:      return "funcref";
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

std::string_view ExternalKind::getName() const
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

bool ExternalKind::isValid() const
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

std::optional<ExternalKind> ExternalKind::getEncoding(std::string_view name)
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

    return 0;
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

