// Assembler.cpp

#include "Assembler.h"

#include <cctype>

char Assembler::nextChar()
{
    char c = data.nextChar();

    columnNumber++;
    if (c == '\n') {
        columnNumber = 1;
        lineNumber++;
    }

    return c;
}

bool Assembler::readFile(std::istream& stream)
{
    stream.seekg(0, std::ios::end);

    size_t fileSize = stream.tellg();

    stream.seekg(0, std::ios::beg);

    data.resize(fileSize);
    stream.read(data.data(), fileSize);
    return (size_t(stream.gcount()) == fileSize);
}

bool Assembler::readWat(std::istream& stream)
{
    return readFile(stream) &&
        parse();
};

bool isFormatChar(char c)
{
    return c == '\t' || c == '\n' || c == '\r';
}

bool isIdChar(char c)
{
    return std::isprint(c) && c != ' ' && c != '?' && c != ',' && c != ';' && c != '(' && c != ')';
}

bool Assembler::lineComment()
{
    if (peekChar() == ';' && peekChar(1) == ';') {
        bump();
        bump();

        while (!atEnd() && peekChar() != '\n') {
            bump();
        }

        return true;
    }

    return false;
}

bool Assembler::blockComment()
{
    if (peekChar() == '(' && peekChar(1) == ';') {
        bump();
        bump();

        for (;;) {
            if (atEnd()) {
                msgs.error(lineNumber, columnNumber, "Block comment not terminated.");
                break;
            }

            char c = peekChar();

            if (c == ';' && peekChar(1) == ')') {
                bump();
                bump();
                break;
            } else if (c == '(' && peekChar(1) == ';' && blockComment()) {
                //nop
            } else {
                bump();
            }
        }

        return true;
    }

    return false;
}

bool Assembler::whiteSpace()
{
    bool result = false;

    while (!atEnd()) {
        char c = peekChar();

        if (c == ' ' || isFormatChar(c)) {
            bump();
        } else if (c != ';' && c != '(') {
            break;
        } else if (!comment()) {
            break;
        }

        result = true;
    }

    return result;
}

bool Assembler::parseHex()
{
    char c = peekChar();

    if (!isHex(c)) {
        msgs.error(lineNumber, columnNumber, "Invalid hexadecimal character '", c, "'.");
        return false;
    }

    while (isHex(c) || c == '_') {
        bump();
        if (c == '_' && peekChar() == '_') {
            msgs.error(lineNumber, columnNumber, "Consecutive underscoes are not allowed in numbers.");
            return false;
        }

        if (atEnd()) {
            break;
        }

        c = peekChar();
    }

    return true;
}

bool Assembler::parseInteger(bool allowHex)
{
    char c = peekChar();

    if (c == '-' || c == '+') {
        bump();
        c = peekChar();
    }

    if (isNumeric(c)) {
        char c1 = c;
        bump();
        c = peekChar();

        if (allowHex && c1 == '0' && (c == 'x' || c == 'X')) {
            bump();

            if (atEnd()) {
                return false;
            }

            parseHex();
        } else {
            while (isNumeric(c) || c == '_') {
                bump();
                if (c == '_' && peekChar() == '_') {
                    msgs.error(lineNumber, columnNumber, "Consecutive underscoes are not allowed in numbers.");
                    return false;
                }

                if (atEnd()) {
                    break;
                }

                c = peekChar();
            }
        }

        return true;
    }

    return false;
}

bool Assembler::parseString()
{
    if (peekChar() != '"') {
        return false;
    }

    bump();

    while (!atEnd()) {
        if (peekChar() == '\n' || atEnd()) {
            msgs.error(lineNumber, columnNumber, "Unterminated string.");
            return false;
        }

        char c = nextChar();

        if (c == '"') {
            break;
        }

        if (c == '\\' && !atEnd()) {
            c = nextChar();
            if (c == 't' || c == 'n' || c == 'r' || c == '"' || c == '\'') {
                //nop
            } else if (isHex(c) ) {
                if (isHex(peekChar())) {
                    bump();
                } else {
                    msgs.error(lineNumber, columnNumber, "Invalid hexadecomal escape.");
                    return false;
                }
            } else {
                msgs.error(lineNumber, columnNumber, "Invalid escaqpe sequence escape.");
                return false;
            }
        }
    }

    return true;
}

Token::TokenKind Assembler::parseNumber()
{
    Token::TokenKind kind = Token::none;

    if (peekChar() == '0' && (peekChar(1) == 'x' || peekChar(1) == 'X')) {
        parseInteger();
        kind = Token::integer;

        if (peekChar() == '.') {
            bump();
            kind = Token::floating;

            parseHex();
        }

        char c = peekChar();

        if (c == 'p' || c == 'P') {
            bump();
            kind = Token::floating;

            if (peekChar() != '-' && peekChar() != '+') {
                msgs.error(lineNumber, columnNumber, "Invalid floating point number; missing sign before exponent.");
                return Token::none;
            }

            bump();
            parseInteger();
        }
    } else if (parseInteger(false)) {
        kind = Token::integer;

        if (peekChar() == '.') {
            bump();
            kind = Token::floating;
            parseInteger();
        }

        char c = peekChar();

        if (c == 'e' || c == 'E') {
            bump();
            kind = Token::floating;

            if (peekChar() != '-' && peekChar() != '+') {
                msgs.error(lineNumber, columnNumber, "Invalid floating point number; missing sign before exponent.");
                return Token::none;
            }

            bump();
            parseInteger();
        }
    }

    return kind;
}

bool Assembler::tokenize()
{
    whiteSpace();

    while (!atEnd()) {
        auto* startPointer = data.getPointer();
        auto kind = Token::none;
        auto line = lineNumber;
        auto column = columnNumber;
        bool ok = true;
        bool isSeparator = false;

        char c = peekChar();

        if (isAlpha(c)) {
            kind = Token::keyword;

            bump();

            while (isIdChar(peekChar())) {
                bump();
                if (peekChar(-1) == '=') {
                    isSeparator = true;
                    break;
                }
            }
        } else  if (c == '(' || c == ')') {
            kind = Token::parenthesis;
            bump();
            isSeparator = true;
        } else  if (c == '$') {
            bump();
            kind = Token::id;

            while (isIdChar(peekChar())) {
                bump();
            }
        } else if (kind = parseNumber(); kind != Token::none) {
            //nop
        } else if (parseString()) {
            kind = Token::string;
        } else {
            ok = false;
        }

        auto* endPointer = data.getPointer();

        if (!isSeparator) {
            if (!ok || (peekChar() != '(' && peekChar() != ')' && !whiteSpace())) {
                while (peekChar() != '(' && !whiteSpace()) {
                    bump();
                    endPointer++;
                }

                size_t size = endPointer - startPointer;

                msgs.error(line, column, "Invalid token '", std::string_view(startPointer, size), '\'');
            }
        }

        if (kind == Token::string) {
            ++startPointer;
            --endPointer;
        }

        size_t size = endPointer - startPointer;

        tokens.addToken(kind, line, column, std::string_view(startPointer, size));
        whiteSpace();
    }

    return msgs.getErrorCount() == 0;
}

static SectionType checkImport(TokenBuffer& tokens, SectionType sectionType)
{
    auto startpos = tokens.getPos();

    tokens.getId();

    if (tokens.getParenthesis('(')) {
        if (auto key = tokens.getKeyword()) {
            if (*key == "import") {
                sectionType = SectionType::import;
            }
        }
    }

    tokens.setPos(startpos);
    return sectionType;
}

std::vector<Assembler::SectionElementIndex> Assembler::findSectionEntries(bool isModule)
{
    std::vector<SectionElementIndex> result;

    while (!tokens.atEnd()) {
        if (isModule && tokens.getPos() == tokens.size() - 1 && tokens.getParenthesis(')')) {
            break;
        }

        auto startPos = tokens.getPos();

        if (tokens.getParenthesis('(')) {
            SectionType sectionType = SectionType::max + 1;

            if (auto key = tokens.getKeyword()) {
                if      (*key == "type") {
                    sectionType = SectionType::type;
                } else if (*key == "memory") {
                    sectionType = checkImport(tokens, SectionType::memory);
                } else if (*key == "table") {
                    sectionType = checkImport(tokens, SectionType::table);
                } else if (*key == "global") {
                    sectionType = checkImport(tokens, SectionType::global);
                } else if (*key == "func") {
                    sectionType = checkImport(tokens, SectionType::function);
                } else if (*key == "start") {
                    sectionType = SectionType::start;
                } else if (*key == "export") {
                    sectionType = SectionType::export_;
                } else if (*key == "import") {
                    sectionType = SectionType::import;
                } else if (*key == "datacount") {
                    sectionType = SectionType::dataCount;
                } else if (*key == "elem") {
                    sectionType = SectionType::element;
                } else if (*key == "data") {
                    sectionType = SectionType::data;
                }
            }

            if (sectionType == SectionType::max + 1) {
                msgs.expected(tokens.peekToken(),
                        "one of 'type' 'memory' 'table' 'global' 'func' 'start' 'export' 'import' 'dataCount' 'elem' or 'data'");
            } else {
                result.emplace_back(sectionType, startPos);
            }

        } else {
            msgs.expected(tokens.peekToken(), "'('");
        }

        tokens.recover();
    }

    return result;
}

std::vector<size_t> Assembler::findSectionPositions(
        const std::vector<Assembler::SectionElementIndex>& entries, SectionType type)
{
    std::vector<size_t> result;

    for (const auto& entry : entries) {
        if (entry.type == type) {
            result.push_back(entry.pos);
        }
    }
    
    return result;
}

bool Assembler::checkSemantics()
{
    context.getModule()->makeDataCountSection();

    CheckErrorHandler error;
    CheckContext checkContext(context, error);

    if (!checkContext.checkSemantics()) {
        errorCount += checkContext.msgs().getErrorCount();
        warningCount += checkContext.msgs().getWarningCount();
        return false;
    } else {
        return true;
    }
}

bool Assembler::parse()
{
    return tokenize() &&
        doParse() &&
        checkSemantics();
}

bool Assembler::doParse()
{
    bool isModule = false;
    auto* module = context.getModule();

    if (tokens.peekToken().isParenthesis('(') && tokens.peekToken(1).isKeyword("module")) {
        tokens.bump(2);
        isModule = true;
    }

    auto entries = findSectionEntries(isModule);

    if (auto positions = findSectionPositions(entries, SectionType::type); !positions.empty()) {
        auto* section = module->requiredTypeSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = TypeDeclaration::parse(context);
            assert(entry != nullptr);
            section->addType(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::import); !positions.empty()) {
        auto* section = module->requiredImportSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = ImportDeclaration::parse(context);
            assert(entry != nullptr);
            section->addImport(entry);
        }
    }

    module->startLocalFunctions();

    if (auto positions = findSectionPositions(entries, SectionType::function); !positions.empty()) {
        auto* sections = module->requiredFunctionSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = FunctionDeclaration::parse(context);
            entries.emplace_back(SectionType::code, tokens.getPos());
            assert(entry != nullptr);
            sections->addFunction(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::table); !positions.empty()) {
        auto* section = module->requiredTableSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = TableDeclaration::parse(context);
            assert(entry != nullptr);
            section->addTable(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::memory); !positions.empty()) {
        auto* section = module->requiredMemorySection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = MemoryDeclaration::parse(context);
            assert(entry != nullptr);
            section->addMemory(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::global); !positions.empty()) {
        auto* section = module->requiredGlobalSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = GlobalDeclaration::parse(context);
            assert(entry != nullptr);
            section->addGlobal(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::code); !positions.empty()) {
        auto* section = module->requiredCodeSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = CodeEntry::parse(context);
            assert(entry != nullptr);
            section->addCode(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::export_); !positions.empty()) {
        auto* section = module->requiredExportSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = ExportDeclaration::parse(context);
            assert(entry != nullptr);
            section->addExport(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::start); !positions.empty()) {
        for (auto position : positions) {
            tokens.setPos(position);

            if (!module->setStartSection(StartSection::parse(context))) {
                tokens.setPos(position);
                context.msgs().error(context.tokens().peekToken(), "Only one start section is allowed.");
            }
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::element); !positions.empty()) {
        auto* section = module->requiredElementSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = ElementDeclaration::parse(context);
            assert(entry != nullptr);
            section->addElement(entry);
        }
    }

    if (auto positions = findSectionPositions(entries, SectionType::data); !positions.empty()) {
        auto* section = module->requiredDataSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = DataSegment::parse(context);
            assert(entry != nullptr);
            section->addSegment(entry);
        }
    }

    return msgs.getErrorCount() == 0;
}

