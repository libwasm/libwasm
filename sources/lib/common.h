// Common.h

#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <iostream>
#include <string_view>

const uint32_t wasmMagic = 0x6d736100;
const uint32_t wasmVersion = 1;
const uint32_t wasmLinkingVersion = 2;
const uint32_t invalidIndex = ~uint32_t(0);
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

unsigned fromHex(char c);

void dumpChars(std::ostream& os, std::string_view chars, size_t startOffset);
std::pair<std::string, std::string> unEscape(std::string_view chars);
void generateChars(std::ostream& os, std::string_view chars);
bool validUtf8(std::string_view string);

int32_t toI32(std::string_view chars);
int64_t toI64(std::string_view chars);
float toF32(std::string_view chars);
double toF64(std::string_view chars);

#endif
