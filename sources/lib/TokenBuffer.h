// TokenBuffer.h

#ifndef TOKEN_BUFFER
#define TOKEN_BUFFER

#include "Token.h"
#include "common.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class TokenBuffer
{
    public:
        size_t getPos() const
        {
            return pos;
        }

        void setPos(size_t p)
        {
            pos = p;
        }

        size_t size() const
        {
            return container.size();
        }

        bool atEnd() const
        {
            return pos == container.size();
        }

        void resize(size_t newSize)
        {
            container.resize(newSize);
        }

        auto& getTokens()
        {
            return container;
        }

        auto& nextToken()
        {
            assert(!atEnd());
            return container[pos++];
        }

        const auto& peekToken(int n = 0) const
        {
            if (pos + n >= size()) {
                static Token token;

                return token;
            }

            return container[pos + n];
        }

        void bump(int count = 1)
        {
            assert(pos + count < size());
            pos += count;
        }

        std::optional<uint32_t> getU32();
        std::optional<int32_t> getI32();
        std::optional<uint64_t> getU64();
        std::optional<int64_t> getI64();
        std::optional<float> getF32();
        std::optional<double> getF64();
        std::optional<std::string_view> getKeyword();
        bool getKeyword(std::string_view v);
        std::optional<std::string_view> getId();
        std::optional<char> getParenthesis();
        bool getParenthesis(char v);
        std::optional<std::string_view> getString();

        std::optional<uint32_t> peekU32(unsigned index = 0);
        std::optional<int32_t> peekI32(unsigned index = 0);
        std::optional<uint64_t> peekU64(unsigned index = 0);
        std::optional<int64_t> peekI64(unsigned index = 0);
        std::optional<float> peekF32(unsigned index = 0);
        std::optional<double> peekF64(unsigned index = 0);
        std::optional<std::string_view> peekKeyword(unsigned index = 0);
        bool peekKeyword(std::string_view v, unsigned index = 0);
        std::optional<std::string_view> peekId(unsigned index = 0);
        std::optional<char> peekParenthesis(unsigned index = 0);
        bool peekParenthesis(char v, unsigned index = 0);
        std::optional<std::string_view> peekString();

        void recover();
        
        void addToken(Token::TokenKind kind, size_t lineNumber, unsigned columnNumber, std::string_view value)
        {
            container.emplace_back(kind, lineNumber, columnNumber, value);
        }

    private:
        size_t pos = 0;
        std::vector<Token> container;
};

#endif
