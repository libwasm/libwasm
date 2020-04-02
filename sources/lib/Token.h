// Token.h

#ifndef TOKEN_H
#define TOKEN_H

#include <string_view>

class Token
{
    public:
        enum TokenKind : unsigned
        {
            none,
            keyword,
            integer,
            floating,
            string,
            id,
            parenthesis,
        };

        Token() = default;

        Token(TokenKind k, size_t l, size_t c, std::string_view v)
          : kind(k), columnNumber(c), lineNumber(l), value(v)
        {
        }

        auto getKind() const
        {
            return kind;
        }

        auto getLineNumber() const
        {
            return lineNumber;
        }

        auto getColumnNumber() const
        {
            return columnNumber;
        }

        auto getValue() const
        {
            return value;
        }

        bool isNone() const
        {
            return kind == none;
        }

        bool isKeyword() const
        {
            return kind == keyword;
        }

        bool isKeyword(std::string_view v) const
        {
            return kind == keyword && value == v;
        }

        bool isInteger() const
        {
            return kind == integer;
        }

        bool isFloating() const
        {
            return kind == floating;
        }

        bool isString() const
        {
            return kind == string;
        }

        bool isString(std::string_view v) const
        {
            return kind == string && value == v;
        }

        bool isId() const
        {
            return kind == id;
        }

        bool isParenthesis() const
        {
            return kind == parenthesis;
        }

        bool isParenthesis(char v) const
        {
            return kind == parenthesis && value[0] == v;
        }

        void dump(std::ostream& os) const;

    private:
        TokenKind kind = none;
        size_t columnNumber = 0;
        size_t lineNumber = 0;
        size_t correspondingParenthesisIndex = 0;
        std::string_view value;

    friend class Assembler;
};


#endif
