// TokenBuffer.cpp

#include "TokenBuffer.h"

#include <cassert>

std::optional<int8_t> TokenBuffer::peekI8(unsigned index)
{
    auto& token = peekToken(index);

    if (token.getKind() == Token::integer) {
        return int8_t(toI32(token.getValue()));
    }

    return {};
}

std::optional<int16_t> TokenBuffer::peekI16(unsigned index)
{
    auto& token = peekToken(index);

    if (token.getKind() == Token::integer) {
        return int16_t(toI32(token.getValue()));
    }

    return {};
}

std::optional<uint32_t> TokenBuffer::peekU32(unsigned index)
{
    auto& token = peekToken(index);

    if (token.getKind() == Token::integer) {
        return uint32_t(toI32(token.getValue()));
    }

    return {};
}

std::optional<int32_t> TokenBuffer::peekI32(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::integer) {
        return toI32(token.getValue());
    }

    return {};
}

std::optional<uint64_t> TokenBuffer::peekU64(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::integer) {
        return uint64_t(toI64(token.getValue()));
    }

    return {};
}

std::optional<int64_t> TokenBuffer::peekI64(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::integer) {
        return toI64(token.getValue());
    }

    return {};
}

std::optional<float> TokenBuffer::peekF32(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::floating) {
        return toF32(token.getValue());
    }

    return {};
}

std::optional<double> TokenBuffer::peekF64(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::floating) {
        return toF64(token.getValue());
    }

    return {};
}

std::optional<std::string_view> TokenBuffer::peekKeyword(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::keyword) {
        return token.getValue();
    }

    return {};
}

bool TokenBuffer::peekKeyword(std::string_view v, unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::keyword && token.getValue() == v) {
        return true;
    }

    return false;
}

std::optional<char> TokenBuffer::peekParenthesis(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::parenthesis) {
        return token.getValue()[0];
    }

    return {};
}

bool TokenBuffer::peekParenthesis(char v, unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::parenthesis && token.getValue()[0] == v) {
        return true;
    }

    return false;
}

std::optional<std::string_view> TokenBuffer::peekId(unsigned index)
{
    assert(!atEnd());
    auto& token = peekToken(index);

    if (token.getKind() == Token::id) {
        return token.getValue();
    }

    return {};
}

std::optional<std::string_view> TokenBuffer::peekString()
{
    assert(!atEnd());
    auto& token = peekToken();

    if (token.getKind() == Token::string) {
        return token.getValue();
    }

    return {};
}

std::optional<int8_t> TokenBuffer::getI8()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::integer) {
        pos++;
        return int8_t(toI32(token.getValue()));
    }

    return {};
}

std::optional<int16_t> TokenBuffer::getI16()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::integer) {
        pos++;
        return int16_t(toI32(token.getValue()));
    }

    return {};
}

std::optional<uint32_t> TokenBuffer::getU32()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::integer) {
        pos++;
        return uint32_t(toI32(token.getValue()));
    }

    return {};
}

std::optional<int32_t> TokenBuffer::getI32()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::integer) {
        pos++;
        return toI32(token.getValue());
    }

    return {};
}

std::optional<uint64_t> TokenBuffer::getU64()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::integer) {
        pos++;
        return uint64_t(toI64(token.getValue()));
    }

    return {};
}

std::optional<int64_t> TokenBuffer::getI64()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::integer) {
        pos++;
        return toI64(token.getValue());
    }

    return {};
}

std::optional<float> TokenBuffer::getF32()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (auto kind = token.getKind(); kind == Token::floating || kind == Token::integer) {
        pos++;
        return toF32(token.getValue());
    }

    return {};
}

std::optional<double> TokenBuffer::getF64()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (auto kind = token.getKind(); kind == Token::floating || kind == Token::integer) {
        pos++;
        return toF64(token.getValue());
    }

    return {};
}

std::optional<std::string_view> TokenBuffer::getKeyword()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::keyword) {
        pos++;
        return token.getValue();
    }

    return {};
}

bool TokenBuffer::getKeyword(std::string_view v)
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::keyword && token.getValue() == v) {
        pos++;
        return true;
    }

    return false;
}

std::optional<char> TokenBuffer::getParenthesis()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::parenthesis) {
        pos++;
        return token.getValue()[0];
    }

    return {};
}

bool TokenBuffer::getParenthesis(char v)
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::parenthesis && token.getValue()[0] == v) {
        pos++;
        return true;
    }

    return false;
}

std::optional<std::string_view> TokenBuffer::getId()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::id) {
        pos++;
        return token.getValue();
    }

    return {};
}

std::optional<std::string_view> TokenBuffer::getString()
{
    assert(!atEnd());
    auto& token = container[pos];

    if (token.getKind() == Token::string) {
        pos++;
        return token.getValue();
    }

    return {};
}

void TokenBuffer::recover()
{
    unsigned depth = 1;

    while (!atEnd()) {
        auto& token = nextToken();

        if (token.getKind() == Token::parenthesis) {
            char value = token.getValue()[0];

            if (value == '(') {
                depth++;
            } else {
                assert(depth > 0);
                if (--depth == 0) {
                    return;
                }
            }
        }
    }

}

