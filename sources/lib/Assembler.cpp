// Assembler.cpp

#include "Assembler.h"

#include "Disassembler.h"
#include "parser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <tuple>
#include <map>

namespace libwasm
{

char Assembler::nextChar()
{
    char c = data.nextChar();

    if (c == '\n') {
        lastNlPointer = data.getPointer();
        lineNumber++;
    }

    return c;
}

bool Assembler::readFile(std::istream& stream)
{
    return data.readFile(stream);
}

bool Assembler::lineComment()
{
    if (peekChars(";;")) {
        bump(2);

        while (!atEnd() && peekChar() != '\n') {
            bump();
        }

        return true;
    }

    return false;
}

bool Assembler::blockComment()
{
    if (peekChars("(;")) {
        bump(2);

        for (;;) {
            if (atEnd()) {
                msgs.error(lineNumber, getColumnNumber(), "Block comment not terminated.");
                break;
            }

            char c = peekChar();

            if (c == ';' && peekChar(1) == ')') {
                bump(2);
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

void Assembler::whiteSpace()
{
    for (;;) {
        data.skipChars(' ');

        while (peekChar() == '\n') {
            nextChar();
            data.skipChars(' ');
        }

        char c = peekChar();

        if (c == ';' && peekChar(1) == ';' && lineComment()) {
            // nop
        } else if (c == '(' && peekChar(1) == ';' && blockComment()) {
            // nop
        } else if (c == '\t' || c == '\r') {
            bump();
        } else {
            return;
        }
    }
}

bool Assembler::parseHex()
{
    char c = peekChar();

    if (!isHex(c)) {
        msgs.error(lineNumber, getColumnNumber(), "Invalid hexadecimal character '", c, "'.");
        return false;
    }

    while (isHex(c) || c == '_') {
        bump();
        if (c == '_') {
            if (peekChar() == '_') {
                msgs.error(lineNumber, getColumnNumber(), "Consecutive underscoes are not allowed in numbers.");
                return false;
            } else if (!isHex(peekChar())) {
                msgs.error(lineNumber, getColumnNumber(), "Underscores can only separate digits.");
            }
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
    auto startPos = data.getPos();

    if (c == '-' || c == '+') {
        c = data.bumpPeekChar();
    }

    if (isNumeric(c)) {
        char c1 = c;
        c = data.bumpPeekChar();

        if (allowHex && c1 == '0' && c == 'x') {
            bump();

            if (atEnd()) {
                return false;
            }

            parseHex();
        } else {
            while (isNumeric(c) || c == '_') {
                bump();
                if (c == '_') {
                    if (peekChar() == '_') {
                        msgs.error(lineNumber, getColumnNumber(), "Consecutive underscoes are not allowed in numbers.");
                        return false;
                    } else if (!isNumeric(peekChar())) {
                        msgs.error(lineNumber, getColumnNumber(), "Underscores can only separate digits.");
                    }
                }

                if (atEnd()) {
                    break;
                }

                c = peekChar();
            }
        }

        return true;
    }

    data.setPos(startPos);
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
            msgs.error(lineNumber, getColumnNumber(), "Unterminated string.");
            return false;
        }

        char c = nextChar();

        if (c == '"') {
            break;
        }

        if (c == '\\' && !atEnd()) {
            c = nextChar();
            if (isHex(c) ) {
                if (isHex(peekChar())) {
                    bump();
                } else {
                    msgs.error(lineNumber, getColumnNumber(), "Invalid hexadecomal escape.");
                    return false;
                }
            }
        }
    }

    return true;
}

Token::TokenKind Assembler::parseNumber()
{
    Token::TokenKind kind = Token::none;

    if (peekChars("0x")) {
        parseInteger();
        kind = Token::integer;

        if (peekChar() == '.') {
            bump();
            kind = Token::floating;

            if (isHex(peekChar())) {
                parseHex();
            }
        }

        char c = peekChar();

        if (c == 'p' || c == 'P') {
            bump();
            kind = Token::floating;
            parseInteger();
        }
    } else if (parseInteger(false)) {
        kind = Token::integer;

        if (peekChar() == '.') {
            bump();
            kind = Token::floating;

            if (isNumeric(peekChar())) {
                parseInteger(false);
            }
        }

        char c = peekChar();

        if (c == 'e' || c == 'E') {
            bump();
            kind = Token::floating;
            parseInteger(false);
        }
    }

    return kind;
}

bool Assembler::parseNan()
{
    if (peekChars("nan")) {
        if (peekChar(3) == ':') {
            bump(4);
            if (peekChars("canonical")) {
                bump(9);
            } else if (peekChars("arithmetic")) {
                bump(10);
            } else {
                (void) parseInteger();
            }

            return true;
        } else if (!isIdChar(peekChar(3))) {
            bump(3);
            return true;
        }
    }

    return false;
}

bool Assembler::parseInf()
{
    if (peekChars("inf") && !isIdChar(peekChar(3))) {
        bump(3);
        return true;
    }

    return false;
}

void Assembler::skipIdChars()
{
    auto c = peekChar();

    while (isIdChar(c)) {
        c = data.bumpPeekChar();
    }
}

bool Assembler::tokenize()
{
    whiteSpace();
    std::vector<size_t> parenthesisStack;

    for (char c = peekChar(); c != 0; c = peekChar()) {
        auto* startPointer = data.getPointer();
        auto kind = Token::none;
        auto line = lineNumber;
        auto column = getColumnNumber();

        if (isLowerAlpha(c)) {
            if ((c == 'n' && parseNan()) || ( c == 'i' && parseInf())) {
                kind = Token::floating;
            } else if (c == 'o' && peekChars("offset=")) {
                bump(7);
                kind = Token::keyword;
            } else if (c == 'a' && peekChars("align=")) {
                bump(6);
                kind = Token::keyword;
            } else {
                bump();
                skipIdChars();

                kind = Token::keyword;
            }
        } else  if (c == '(' || c == ')') {
            kind = Token::parenthesis;
            bump();
        } else if (c == '-' || c == '+') {
            bump();

            if (auto c1 = peekChar(); (c1 == 'n' && parseNan()) || ( c1 == 'i' && parseInf())) {
                kind = Token::floating;
            } else if (kind = parseNumber(); kind != Token::none) {
                //nop
            } else {
                if (isIdChar(peekChar())) {
                    kind = Token::reserved;

                    skipIdChars();
                }
            }
        } else if (kind = parseNumber(); kind != Token::none) {
            if (isIdChar(peekChar())) {
                bump();
                kind = Token::reserved;

                skipIdChars();
            }
        } else if (parseString()) {
            kind = Token::string;
        } else  if (c == '$') {
            bump();
            kind = Token::id;

            skipIdChars();
        } else if (isIdChar(c)) {
            bump();
            kind = Token::reserved;

            skipIdChars();
        } else {
            msgs.error(line, column, "Invalid character '", c, '\'');
            bump();
        }

        auto* endPointer = data.getPointer();

        if (kind == Token::string) {
            ++startPointer;
            --endPointer;
        } else if (kind == Token::id) {
            ++startPointer;
        }

        size_t size = endPointer - startPointer;

        tokens.addToken(kind, line, column, std::string_view(startPointer, size));

        if (c == '(') {
            parenthesisStack.push_back(tokens.size() - 1);
        } else if (c == ')') {
            if (!parenthesisStack.empty()) {
                auto otherIndex = parenthesisStack.back();

                parenthesisStack.pop_back();
                tokens.getTokens().back().correspondingParenthesisIndex = otherIndex;
                tokens.getTokens()[otherIndex].correspondingParenthesisIndex = tokens.size() - 1;
            } else {
                msgs.error(line, column, "Unmatched ')'.");
            }
        }

        whiteSpace();
    }

    return msgs.getErrorCount() == 0;
}

static SectionType checkImport(TokenBuffer& tokens, SectionType sectionType)
{
    auto startpos = tokens.getPos();

    tokens.getId();

    while (tokens.peekParenthesis('(') && tokens.peekKeyword("export", 1)) {
        auto& token = tokens.peekToken();

        tokens.setPos(token.getCorrespondingIndex() + 1);
    }

    if (tokens.peekParenthesis('(') && tokens.peekKeyword("import", 1)) {
        sectionType = SectionType::import;
    }

    tokens.setPos(startpos);
    return sectionType;
}

std::vector<Assembler::SectionElementIndex>
    Assembler::findSectionEntries(size_t startPos, size_t endPos)
{
    std::vector<SectionElementIndex> result;

    tokens.setPos(startPos);

    while ((startPos = tokens.getPos()) != endPos) {
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
    CheckErrorHandler error(msgs.getErrorStream());
    CheckContext checkContext(context, error);

    if (!checkContext.checkSemantics()) {
        errorCount += checkContext.msgs().getErrorCount();
        warningCount += checkContext.msgs().getWarningCount();
        return false;
    } else {
        return true;
    }
}

bool Assembler::parseModule(size_t startPos, size_t endPos)
{
    auto errorCount = msgs.getErrorCount();
    auto entries = findSectionEntries(startPos, endPos);

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
    module->startLocalMemories();
    module->startLocalTables();
    module->startLocalEvents();
    module->startLocalGlobals();

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

    if (auto positions = findSectionPositions(entries, SectionType::data); !positions.empty()) {
        auto* section = module->requiredDataSection();

        for (auto position : positions) {
            tokens.setPos(position);

            auto entry = DataSegment::parse(context);
            assert(entry != nullptr);
            section->addSegment(entry);
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

    tokens.setPos(endPos);
    checkSemantics();

    return errorCount == msgs.getErrorCount();
}

bool Assembler::doParseScript()
{
    script = std::make_shared<Script>();

    if (tokens.getParenthesis('(')) {
        if (auto key = tokens.getKeyword()) {
            if (*key == "type" ||
                    *key == "memory" ||
                    *key == "table" ||
                    *key == "global" ||
                    *key == "func" ||
                    *key == "start" ||
                    *key == "export" ||
                    *key == "import" ||
                    *key == "datacount" ||
                    *key == "elem" ||
                    *key == "code" ||
                    *key == "data") {

                tokens.setPos(0);

                module = std::make_shared<Module>();
                context.setModule(module);

                size_t startPos = 0;
                size_t endPos = tokens.size();

                if (auto id = tokens.getId()) {
                    module->setId(*id);
                }

                if (parseModule(startPos, endPos)) {
                    script->addModule(module);
                }

                return msgs.getErrorCount() == 0;
            }
        }
    }

    tokens.setPos(0);

    std::map<std::string_view, int> ignoreds;

    while (!tokens.atEnd()) {
        if (startClause(context, "module")) {
            module = std::make_shared<Module>();
            context.setModule(module);

            auto endPos = tokens.peekToken(-2).getCorrespondingIndex();

            if (auto id = tokens.getId()) {
                module->setId(*id);
            }

            if (tokens.getKeyword("binary")) {
                std::string code;

                while (auto str = tokens.getString()) {
                    code.append(context.unEscape(*str));
                }

                std::stringstream stream(code);
                Disassembler disassembler(stream);

                if (disassembler.isGood()) {
                    script->addModule(disassembler.getModule());
                } else {
                    std::cerr << "Unable to read binary module" << std::endl;
                }
            } else {
                auto startPos = tokens.getPos();

                if (parseModule(startPos, endPos)) {
                    script->addModule(module);
                }
            }

            requiredCloseParenthesis(context);
        } else if (auto* assertReturn = AssertReturn::parse(context); assertReturn != nullptr) {
            auto p = std::shared_ptr<AssertReturn>(assertReturn);

            script->addAssertReturn(p);
        } else if (auto* invoke = Invoke::parse(context); invoke != nullptr) {
            auto p = std::shared_ptr<Invoke>(invoke);

            script->addInvoke(p);
        } else if (startClause(context, "register")) {
            if (auto name = requiredString(context); !name.empty()) {
                if (module->getId().empty()) {
                    module->setId(name);
                }
            }

            (void)tokens.getId();
            requiredCloseParenthesis(context);
        } else if (tokens.getParenthesis('(')) {
            ++ignoreds[tokens.peekToken().getValue()];
            tokens.recover();
        } else {
            msgs.error(tokens.peekToken(), "Expected '('.");
        }
    }

    for (const auto& ignored : ignoreds) {
        std::cout << "   " << ignored.second << " '" << ignored.first  << "' Ignored.\n";
    }

    return msgs.getErrorCount() == 0;
}

bool Assembler::parse()
{
    return tokenize() &&
        doParseScript();
}

void Assembler::write(std::ostream& os)
{
    if (auto* exportSection = module->getExportSection(); exportSection != nullptr) {
        auto& exports = exportSection->getExports();

        using exporType = std::unique_ptr<ExportDeclaration>;

        std::sort(exports.begin(), exports.end(),
                [](const exporType& x, const exporType& y) {
                    return std::tuple(x->getLineNumber(), x->getColumnNumber()) <
                           std::tuple(y->getLineNumber(), y->getColumnNumber());
                });
    }

    context.write(os);
}

};
