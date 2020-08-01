// Common.h

#ifndef COMMON_H
#define COMMON_H

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <string>

namespace libwasm
{
using v128_t = std::array<uint8_t, 16>;

const uint32_t wasmMagic = 0x6d736100;
const uint32_t wasmVersion = 1;
const uint32_t wasmLinkingVersion = 2;
const uint32_t invalidIndex = ~uint32_t(0);
const uint32_t memoryPageSize = 65536;
const auto invalidSection = ~size_t(0);

inline char hexChar(unsigned c)
{
    return (c < 10) ? char(c + '0') : char(c - 10 + 'a');
}

inline bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z');
}

inline bool isLowerAlpha(char c)
{
    return (c >= 'a' && c <= 'z');
}

inline bool isNumeric(char c)
{
    return (c >= '0' && c <= '9');
}

inline bool isAlphaNumeric(char c)
{
    return isAlpha(c) || isNumeric(c);
}

inline bool isHex(char c)
{
    return isNumeric(c) ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

inline char toUpper(char c)
{
    if (c >= 'a' && c <= 'z') {
        return char(c - ('a' - 'A'));
    } else {
        return c;
    }
}

inline bool isIdChar(char c)
{
    if (c >= '^' && c <= 'z') {
        return true;
    }

    if (c >= '*' && c <= 'Z') {
        return c != ',' && c != ';';
    }

    if (c >= '!' && c <= '\'') {
        return true;
    }

    return c == '\\' || c == '|' || c == '~';
}

unsigned hash(std::string_view value);

unsigned fromHex(char c);

bool isBinary(std::istream& stream);
void dumpChars(std::ostream& os, std::string_view chars, size_t startOffset);
std::pair<std::string, std::string> unEscape(std::string_view chars);
void generateChars(std::ostream& os, std::string_view chars);
void generateCChars(std::ostream& os, std::string_view chars);
bool validUtf8(std::string_view string);

int64_t toI64(std::string_view chars);
inline int32_t toI32(std::string_view chars)
{
    return int32_t(toI64(chars));
}

std::string normalize(std::string_view chars);
float toF32(std::string_view chars);
double toF64(std::string_view chars);

std::string addFinalPoint(std::string_view string);
std::string toString(uint32_t value);
std::string toHexString(uint64_t value);
std::string toString(float value, bool hexfloat = false);
std::string toString(double value, bool hexfloat = false);

std::string cName(std::string_view name);
std::string makeResultName(unsigned label, size_t index = 0);

};

#endif
