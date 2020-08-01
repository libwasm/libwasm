// Common.cpp

#include "common.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace libwasm
{

bool isBinary(std::istream& stream)
{
    stream.seekg(0, std::ios::end);

    if (auto fileSize = stream.tellg(); fileSize < 4) {
        return false;
    }

    union
    {
        char chars[4];
        uint8_t p[4];
    };

    stream.seekg(0, std::ios::beg);
    stream.read(chars, 4);

    unsigned i = p[0] | (p[1] << 8) | (p[2] << 16) | p[3] << 24;

    stream.seekg(0, std::ios::beg);
    return i == wasmMagic;
}

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

void generateCChars(std::ostream& os, std::string_view chars)
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
                    os << "\\x";
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

        if ((c & 0x80) == 0) {
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

std::string normalize(std::string_view chars)
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

    if (chars[pos] == '0') {
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
            i |= 0x7ff0000000000000ull;
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

std::string cName(std::string_view name)
{
    static const std::string_view reserved[] = {
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "ceil",
        "char",
        "char16_t",
        "char32_t",
        "class",
        "compl",
        "concept",
        "const",
        "const_cast",
        "constexpr",
        "continue",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "false",
        "float",
        "floor",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "register",
        "reinterpret_cast",
        "requires",
        "return",
        "round",
        "short",
        "signed",
        "sizeof",
        "static",
        "static_assert",
        "static_cast",
        "struct",
        "switch",
        "template",
        "this",
        "thread_local",
        "throw",
        "true",
        "try",
        "typedef",
        "typeid",
        "typename",
        "union",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
    };

    std::string result;

    if (!name.empty()) {
        if (!isAlpha(name[0]) && name[0] != '_') {
            result = '_';
        }

        for (auto c : name) {
            if (!isAlphaNumeric(c) && c != '_') {
                result += '_';
                result += hexChar((c >> 4) & 0xf);
                result += hexChar(c & 0xf);
            } else {
                result += c;
            }
        }

        if (auto it = std::lower_bound(std::begin(reserved), std::end(reserved), result);
                it != std::end(reserved) && result == *it) {
            result += '_';
        }
    }

    return result;
}

std::string toString(uint32_t value)
{
    std::string result;

    if (value == 0) {
        result = '0';
    } else {
        while (value != 0) {
            result += char((value % 10) + '0');
            value /= 10;
        }

        std::reverse(result.begin(), result.end());
    }

    return result;
}

std::string toHexString(uint64_t value)
{
    std::string result;

    if (value == 0) {
        result = '0';
    } else {
        while (value != 0) {
            auto digit = value & 0xf;
            value >>= 4;
            
            result += char((digit < 10) ? (digit + '0') : (digit - 10 + 'a'));
        }

        std::reverse(result.begin(), result.end());
    }

    return result;
}

std::string addFinalPoint(const std::string_view string)
{
    std::string result(string.begin(), string.end());

    for (auto c : string) {
        if (isAlpha(c) || c == '.') {
            return result;
        }
    }

    result += ".0";
    return result;
}

std::string toString(float value, bool hexfloat)
{
    std::string result;

    union
    {
        uint32_t i;
        float f;
    };

    f = value;

    if ((i & 0x7f800000) == 0x7f800000) {
        if ((i & 0x80000000) != 0) {
            result =  '-';
            i &= 0x7fffffff;
        }

        if (i == 0x7f800000) {
            result += "INFINITY";
        } else if (i != 0x7fc00000) {
            result += "nanF32(";
            result += "0x" + toHexString(i & 0x7fffff) + ")";
        } else {
            result += "NAN";
        }
    } else {
        std::stringstream stream;

        if (hexfloat) {
            stream << std::hexfloat << value;
            result += stream.str() + 'f';
        } else {
            stream << value;
            result += addFinalPoint(stream.str()) + 'f';
        }
    }

    return result;
}

std::string toString(double value, bool hexfloat)
{
    std::string result;

    union
    {
        uint64_t i;
        double f;
    };

    f = value;

    if ((i & 0x7ff0000000000000ull) == 0x7ff0000000000000ull) {
        if ((i & 0x8000000000000000) != 0) {
            result =  '-';
            i &= 0x7fffffffffffffff;
        }

        if (i == 0x7ff0000000000000ull) {
            result +=  "INFINITY";
        } else if (i != 0x7ff8000000000000ull) {
            result += "nanF64(";
            result += "0x" + toHexString(i & 0xfffffffffffffull) + ")";
        } else {
            result += "NAN";
        }
    } else  {
        std::stringstream stream;

        if (hexfloat) {
            stream << std::hexfloat << value;
            result += stream.str();
        } else {
            stream << value;
            result += addFinalPoint(stream.str());
        }
    }

    return result;
}

std::string makeResultName(unsigned label, size_t index)
{
    std::string result("_result_");

    result += toString(label);

    if (index != 0) {
        result += '_';
        result += toString(uint32_t(index));
    }

    return result;
}

};
