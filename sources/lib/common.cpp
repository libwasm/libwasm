// Common.cpp

#include "common.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <iomanip>

void dumpChars(std::ostream& os, std::string_view chars, size_t startOffset)
{
    auto flags = os.flags();
    char alpha[17];
    char buffer[50];

    std::fill_n(buffer, 49, ' ');
    buffer[49] = 0;
    alpha[16] = 0;

    os << std::hex;
    os << '\n' << std::setw(8) << std::setfill('0') << startOffset << ": ";
    startOffset += 16;

    unsigned i = 0;
    auto start = chars.data();
    auto size = chars.size();

    for (unsigned j = 0; size-- !=  0; ++i) {
        if (i == 16) {
            os << buffer << "    " << alpha;
            os << '\n' << std::setw(8) << std::setfill('0') << startOffset << ": ";
            startOffset += 16;
            std::fill_n(buffer, 49, ' ');
            i = 0;
            j = 0;
        } else if (i == 8) {
            j++;
        }

        unsigned c = uint8_t(*(start++));

        alpha[i] = std::isprint(c) ? char(c) : '.';

        j++;
        buffer[j++] = hexChar(c >> 4);
        buffer[j++] = hexChar(c & 0xf);
    }

    alpha[i] = 0;
    os << buffer << "    " << alpha << '\n';

    os.flags(flags);
}

unsigned fromHex(char c)
{
    if (isNumeric(c)) {
        return c -'0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' +10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' +10;
    } else {
        return 0;
    }
}

std::pair<std::string, std::string> unEscape(std::string_view chars)
{
    std::string result;
    std::string error;

    for (auto b = chars.begin(), e = chars.end(); b != e; ) {
        char c = *b++;

        if (c == '\\' && b < e) {
            if (b == e) {
                error = "Invald escape sequence.";
                break;
            }

            c = *b++;
            switch(c) {
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case '"':  result += '\"'; break;
                case '\'': result += '\''; break;
                case '\\': result += '\\'; break;
                default:
                    if (!isHex(c)) {
                        error = "Invald escape sequence.";
                        break;
                    }

                    if (b == e) {
                        error = "Invald escape sequence.";
                        break;
                    }

                    char c1 = *b++;

                    if (!isHex(c1)) {
                        error = "Invald escape sequence.";
                        break;
                    }

                    result += char((fromHex(c) << 4) | fromHex(c1)); 
            }

        } else {
            result += c;
        }
    }

    return { error, result };
}

void generateChars(std::ostream& os, std::string_view chars)
{
    for (auto c : chars) {
        if (std::isprint(c)) {
            os << c;
        } else {
            switch(c) {
                case '\n' : os << "\\n"; break;
                case '\r' : os << "\\r"; break;
                case '\t' : os << "\\t"; break;
                case '\"' : os << "\\\""; break;
                case '\'' : os << "\\'"; break;
                case '\\' : os << "\\\\"; break;
                default:
                    os << '\\';
                    os << hexChar((c >> 4) & 0xf);
                    os << hexChar(c & 0xf);
            }
        }
    }
}

bool validUtf8(std::string_view string)
{
    const char* p = string.data();
    const char* endP = p + string.size();

    while (p < endP) {
        char c = *(p++);

        if (c & 0x80 == 0) {
            // nop
        } else if ((c & 0xd0) == 0xc0) {
            if (p == endP || (*(p++) & 0xc0) != 0x80) {
                return false;
            }
        } else if ((c & 0xf0) == 0xe0) {
            if (p == endP || (*(p++) & 0xc0) != 0x80) {
                return false;
            }

            if (p == endP || (*(p++) & 0xc0) != 0x80) {
                return false;
            }
        } else if ((c & 0xf8) == 0xf0) {
            if (p == endP || (*(p++) & 0xc0) != 0x80) {
                return false;
            }

            if (p == endP || (*(p++) & 0xc0) != 0x80) {
                return false;
            }

            if (p == endP || (*(p++) & 0xc0) != 0x80) {
                return false;
            }
        } else {
            return false;
        }
    }

    return false;
}

static std::string removeUnderscores(std::string_view chars)
{
    std::string result;

    result.reserve(chars.size());

    for (char c : chars) {
        if (c != '_') {
            result += c;
        }
    }

    return result;
}

int64_t toI64(std::string_view chars)
{
    if (chars[0] == '+') {
        chars.remove_prefix(1);
    }

    auto string = removeUnderscores(chars);

    return strtol(string.data(), nullptr, 0);
}

int32_t toI32(std::string_view chars)
{
    if (chars[0] == '+') {
        chars.remove_prefix(1);
    }

    auto string = removeUnderscores(chars);

    return int32_t(strtol(string.data(), nullptr, 0));
}

double toF64(std::string_view chars)
{
    if (chars[0] == '+') {
        chars.remove_prefix(1);
    }

    auto string = removeUnderscores(chars);

    return strtod(string.c_str(), nullptr);
}

float toF32(std::string_view chars)
{
    if (chars[0] == '+') {
        chars.remove_prefix(1);
    }

    auto string = removeUnderscores(chars);

    return strtof(string.c_str(), nullptr);
}

