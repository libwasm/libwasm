// Assembler.h

#ifndef PARSER_H
#define PARSER_H

#include "BackBone.h"
#include "Context.h"
#include "DataBuffer.h"
#include "ErrorHandler.h"
#include "Token.h"
#include "TokenBuffer.h"
#include "common.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

class Assembler
{
    public:
        Assembler(std::istream& stream)
          : context(tokens, msgs), sections(context.getSections())
        {
            good = readWat(stream);
        }

        ~Assembler() = default;

        bool isGood() const
        {
            return good;
        }

        void show(std::ostream& os, unsigned flags)
        {
            context.show(os, flags);
        }

        void generate(std::ostream& os)
        {
            context.generate(os);
        }

        void write(std::ostream& os)
        {
            context.write(os);
        }

        auto getErrorCount() const
        {
            return msgs.getErrorCount();
        }

        auto getWarningCount() const
        {
            return msgs.getWarningCount();
        }

    private:
        struct SectionElementIndex
        {
            SectionElementIndex(SectionType t, size_t p)
              : type(t), pos(p)
            {
            }

            SectionType type;
            size_t pos;
        };

        bool readWat(std::istream& stream);
        bool readFile(std::istream& stream);
        bool tokenize();
        std::vector<SectionElementIndex> findSectionEntries(bool module);
        std::vector<size_t> findSectionPositions(
                const std::vector<Assembler::SectionElementIndex>& entries, SectionType type);

        bool atEnd() const
        {
            return data.atEnd();
        }

        char nextChar();
        char peekChar() const
        {
            return data.peekChar();
        }

        char peekChar(size_t n) const
        {
            return data.peekChar(n);
        }

        void bump()
        {
            (void) nextChar();
        }

        bool whiteSpace();
        bool blockComment();
        bool lineComment();
        bool comment()
        {
            return lineComment() || blockComment();
        }

        bool parse();
        bool doParse();
        bool parseInteger(bool allowHex = true);
        bool parseString();
        bool parseHex();
        Token::TokenKind parseNumber();

        bool good = false;
        unsigned columnNumber = 1;
        size_t lineNumber = 1;
        DataBuffer data;
        TokenBuffer tokens;
        SourceErrorHandler msgs;

        SourceContext context;
        std::vector<std::shared_ptr<Section>>& sections;
};

#endif
