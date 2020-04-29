// Common.cpp

#include "common.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <iomanip>

namespace libwasm
{

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
        switch(c) {
            case '\n' : os << "\\n"; break;
            case '\r' : os << "\\r"; break;
            case '\t' : os << "\\t"; break;
            case '\"' : os << "\\\""; break;
            case '\'' : os << "\\'"; break;
            case '\\' : os << "\\\\"; break;
            default:
                if (std::isprint(c)) {
                    os << c;
                } else {
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

static std::string normalize(std::string_view chars)
{
    if (chars[0] == '+') {
        chars.remove_prefix(1);
    }

    std::string result;

    result.reserve(chars.size());

    size_t pos = 0;
    size_t size = chars.size();

    if (chars[pos] == '-') {
        result = '-';
        ++pos;
    }

    if (chars[0] == '0') {
        pos = 1;

        while (pos < size && (chars[pos] == '0' || chars[pos] == '_')) {
            ++pos;
        }

        if (pos == size || !isNumeric(chars[pos])) {
            result += '0';
        }
    }

    chars.remove_prefix(pos);

    for (char c : chars) {
        if (c != '_') {
            result += c;
        }
    }

    return result;
}

// The strings pased to this function have been validated by the parser.
// Therefore, we can cut some corners here.
int64_t toI64(std::string_view chars)
{
    bool negative = false;
    uint64_t u64 = 0;

    if (chars[0] == '+') {
        chars.remove_prefix(1);
    } else if (chars[0] == '-') {
        negative = true;
        chars.remove_prefix(1);
    }

    if (chars.size() > 1 && chars[1] == 'x') {
        chars.remove_prefix(2);

        for (char c : chars) {
            if (c != '_') {
                u64 = (u64 << 4) + fromHex(c);
            }
        }
    } else {
        for (char c : chars) {
            if (c != '_') {
                u64 = (u64 * 10) + (c - '0');
            }
        }
    }

    if (negative) {
        return -int64_t(u64);
    } else {
        return int64_t(u64);
    }
}

double toF64(std::string_view chars)
{
    bool negative = false;

    if (chars[0] == '+') {
        chars.remove_prefix(1);
    } else if (chars[0] == '-') {
        negative = true;
        chars.remove_prefix(1);
    }

    auto string = normalize(chars);

    union
    {
        uint64_t i;
        double d;
    };

    if (string[0] == 'n') {
        if (string.size() > 3) {
            i = strtoll(string.c_str() + 4, nullptr, 0);
            i |= 0x7ff0000000000000;
        } else {
            i = 0x7ff8000000000000ull;
        }
    } else if (string[0] == 'i') {
        i = 0x7ff0000000000000ull;
    } else {
        d  = strtod(string.c_str(), nullptr);
    }

    if (negative) {
        d = -d;
    }

    return d;
}

float toF32(std::string_view chars)
{
    bool negative = false;

    if (chars[0] == '+') {
        chars.remove_prefix(1);
    } else if (chars[0] == '-') {
        negative = true;
        chars.remove_prefix(1);
    }

    auto string = normalize(chars);

    union
    {
        uint32_t i;
        float f;
    };

    if (string[0] == 'n') {
        if (string.size() > 3) {
            i = uint32_t(strtol(string.c_str() + 4, nullptr, 0));
            i |= 0x7f800000;
        } else {
            i = 0x7fc00000;
        }
    } else if (string[0] == 'i') {
        i = 0x7f800000;
    } else {
        f  = strtof(string.c_str(), nullptr);
    }

    if (negative) {
        f = -f;
    }

    return f;
}

unsigned hash(std::string_view value)
{
    const unsigned int factor = 9;
    unsigned int h = 0;

    for (auto c : value) {
        h = h * factor + unsigned(c);
    }

    return h * factor + unsigned(value.size());
}

};
