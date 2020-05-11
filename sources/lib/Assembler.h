// Assembler.h

#ifndef PARSER_H
#define PARSER_H

#include "BackBone.h"
#include "Context.h"
#include "DataBuffer.h"
#include "ErrorHandler.h"
#include "Module.h"
#include "Token.h"
#include "TokenBuffer.h"
#include "common.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace libwasm
{
class Assembler
{
    public:
        Assembler(std::istream& stream)
          : context(tokens, msgs), module(context.getModule())
        {
            good = readWat(stream);
        }

        Assembler(std::istream& stream, std::ostream& errorStream)
          : msgs(errorStream), context(tokens, msgs), module(context.getModule())
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
            module->show(os, flags);
        }

        void generate(std::ostream& os)
        {
            module->generate(os);
        }

        void generateS(std::ostream& os)
        {
            module->generateS(os);
        }

        void generateC(std::ostream& os, bool optimized = false)
        {
            module->generateC(os, optimized);
        }

        void write(std::ostream& os);

        auto getErrorCount() const
        {
            return errorCount + msgs.getErrorCount();
        }

        auto getWarningCount() const
        {
            return warningCount + msgs.getWarningCount();
        }

        auto& getContext()
        {
            return context;
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

        void skipIdChars();

        char nextChar();
        char peekChar() const
        {
            return data.peekChar();
        }

        char peekChar(int n) const
        {
            return data.peekChar(n);
        }

        char peekChars(std::string_view chars) const
        {
            return data.peekChars(chars);
        }

        void bump(int count = 1)
        {
            data.bump(count);
        }

        size_t getColumnNumber() const
        {
            return data.getPointer() - lastNlPointer + 1;
        }

        void whiteSpace();
        bool blockComment();
        bool lineComment();
        bool parse();
        bool doParse();
        bool parseInteger(bool allowHex = true);
        bool parseNan();
        bool parseInf();
        bool parseString();
        bool parseHex();
        bool checkSemantics();
        Token::TokenKind parseNumber();

        unsigned errorCount = 0;
        unsigned warningCount = 0;

        bool good = false;
        const char* lastNlPointer = nullptr;
        size_t lineNumber = 1;
        DataBuffer data;
        TokenBuffer tokens;
        SourceErrorHandler msgs;

        SourceContext context;
        Module* module = nullptr;
};
};

#endif
