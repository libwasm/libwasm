// BackBone.cpp

#include "BackBone.h"

#include "CGenerator.h"
#include "ExpressionS.h"
#include "Instruction.h"
#include "Module.h"
#include "common.h"
#include "parser.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std::string_literals;

namespace libwasm
{

Expression:: ~Expression()
{
}

void Expression::addInstruction(Instruction* instruction)
{
    instructions.emplace_back(instruction);
}

static Expression* requiredExpression(SourceContext& context, bool oneInstruction = false)
{
    if (auto* expression = Expression::parse(context, oneInstruction)) {
        return expression;
    }

    context.msgs().error(context.tokens().peekToken(), "Missing or invalid expression.");
    return nullptr;
}

static Expression* makeI32Const0(SourceContext& context)
{
    auto* expression = context.makeTreeNode<Expression>();
    auto* instruction = context.makeTreeNode<InstructionI32>();

    instruction->setOpcode(Opcode::i32__const);
    expression->addInstruction(instruction);

    return expression;
}

static ValueType readValueType(BinaryContext& context)
{
    ValueType result(context.data().getI32leb());

    context.msgs().errorWhen(!result.isValid(), "Invalid value type ", int32_t(result));

    return result;
}

static void writeValueType(BinaryContext& context, ValueType type)
{
    context.data().putI32leb(int32_t(type));
}

static ValueType readElementType(BinaryContext& context)
{
    ValueType result(context.data().getI32leb());
    auto& msgs = context.msgs();

    msgs.errorWhen(!result.isValidRef(), "Invalid element type ", int32_t(result));

    return result;
}

static Limits readLimits(BinaryContext& context)
{
    auto& data = context.data();

    auto flags = data.getU8();
    auto min = data.getU32leb();

    if ((flags & Limits::hasMaxFlag) != 0) {
        auto max = data.getU32leb();

        context.msgs().errorWhen(max < min, "Invalid limits: max (", max,
                ") is less than min (", min, ')');

        return Limits(min, max);
    }

    return Limits(min);
}

static void writeLimits(BinaryContext& context, const Limits& limits)
{
    auto& data = context.data();

    data.putU8(uint8_t(limits.flags));
    data.putU32leb(limits.min);

    if (limits.hasMax()) {
        data.putU32leb(limits.max);
    }
}

static ExternalType readExternalType(BinaryContext& context)
{
    auto& data = context.data();
    auto result = ExternalType(data.getU8());

    context.msgs().errorWhen(!result.isValid(), "Invalid ExternalType ", uint8_t(result));

    return result;
}

static std::string readByteArray(BinaryContext& context)
{
    auto& data = context.data();
    std::string result;

    for (auto length = data.getU32leb(); length > 0; --length) {
        result.push_back(char(data.getU8()));
    }

    return result;
}

static void writeByteArray(BinaryContext& context, std::string_view str)
{
    auto& data = context.data();

    data.putU32leb(uint32_t(str.size()));
    data.append(str);
}

static void shsowFunctionIndex(std::ostream& os, uint32_t index, Module* module)
{
    os << index;

    auto id = module->getFunction(index)->getId();

    if (!id.empty()) {
        os << " \"" << id << "\"";
    }
}

static void makeExport(SourceContext& context, ExternalType type, uint32_t index)
{
    auto* module = context.getModule();

    while (startClause(context, "export")) {
        auto* _export = context.makeTreeNode<ExportDeclaration>();

        _export->setKind(type);
        _export->setNumber(module->nextExportCount());
        _export->setName(requiredString(context));
        _export->setIndex(index);

        module->addExportEntry(_export);

        requiredCloseParenthesis(context);
    }
}

void Limits::generate(std::ostream& os)
{
    os << ' ' << min;

    if (hasMax()) {
        os << ' ' << max;
    }

    if (isShared()) {
        os << " shared";
    }
}

void Limits::show(std::ostream& os)
{
    os << " min=" << min;

    if (hasMax()) {
        os << ", max=" << max;
    }

    if (isShared()) {
        os << ", shared";
    }
}

void Section::setData(Context& context, size_t start, size_t end)
{
    data.append(context.data().data() + start, end - start);
    startOffset = start;
    endOffset = end;
}

void Section::dump(std::ostream& os, BinaryContext& context)
{
    os << '\n' << type << " section:";
    if (data.empty()) {
        os << "*** no binary data available.\n";
    } else {
        dumpChars(os, data, startOffset);
    }
}

CustomSection* CustomSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    CustomSection* result;
    std::string name = readByteArray(context);
    auto& msgs = context.msgs();

    msgs.setSectionName("Custom");

    if (name.find("reloc.") == 0) {
        result = RelocationSection::read(context);
    } else if (name == "linking") {
        result = LinkingSection::read(context, startPos + size);
    } else {
        result = context.makeTreeNode<CustomSection>();
        msgs.warning("Custom section '", name, "' ignored");
    }

    result->setData(context, startPos, startPos + size);
    result->name = name;

    data.setPos(startPos + size);

    if (data.getPos() != startPos + size) {
        msgs.error("Custom section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void CustomSection::check(CheckContext& context)
{
    // nothing to do
}

void CustomSection::generate(std::ostream& os, Module* module)
{
    // nothing to do
}

void CustomSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Custom section " << name << ":\n";
}

RelocationEntry* RelocationEntry::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<RelocationEntry>();

    result->type = RelocationType(data.getU8());
    result->offset = data.getU32leb();
    result->index = data.getU32leb();

    if (hasAddend(result->type)) {
        result->addend = data.getI32leb();
    }

    return result;
}

void RelocationEntry::show(std::ostream& os, Module* module)
{
    auto flags = os.flags();

    os << "  " << type << ", offset=0x" << std::hex << offset << std::dec << ", index=" << index;

    auto* symbol = module->getSymbol(index);
    auto indexName { symbol->getName() };

    if (indexName.empty()) {
        switch (type) {
            case RelocationType::functionIndexLeb:
            {
                if (indexName.empty()) {
                    indexName = module->getFunction(symbol->getIndex())->getId();
                }

                break;
            }

            default:
                break;
        }
    }

    if (!indexName.empty()) {
        os << " \"" << indexName << '\"';
    }

    if (hasAddend(type)) {
        os << ", addend=" << addend;
    }

    os << '\n';
    os.flags(flags);
}

RelocationSection* RelocationSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<RelocationSection>();

    result->targetSectionIndex = data.getU32leb();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->relocations.emplace_back(RelocationEntry::read(context));
    }

    return result;
}

void RelocationSection::check(CheckContext& context)
{
    // nothing to do
}

void RelocationSection::generate(std::ostream& os, Module* module)
{
    // nothing to do
}

void RelocationSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Relocation section " << name << ":";
    os << " target section=" << targetSectionIndex << '\n';

    for (auto& relocation : relocations) {
        relocation->show(os, module);
    }

    os << '\n';
}

LinkingSegmentInfo* LinkingSegmentInfo::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingSegmentInfo>();

    result->name = readByteArray(context);
    result->align = data.getU32leb();
    result->flags = data.getU32leb();

    return result;
}

void LinkingSegmentInfo::show(std::ostream& os, Module* module)
{
    os << "    name=\"" << name << "\", align=" << align << ", flags=" << flags << '\n';
}

LinkingSegmentSubsection* LinkingSegmentSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingSegmentSubsection>();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->infos.emplace_back(LinkingSegmentInfo::read(context));
    }

    return result;
}

void LinkingSegmentSubsection::show(std::ostream& os, Module* module)
{
    os << "  segment info\n";

    for (auto& info : infos) {
        info->show(os, module);
    }
}

LinkingInitFunc* LinkingInitFunc::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingInitFunc>();

    result->priority = data.getU32leb();
    result->functionIndex = data.getU32leb();

    return result;
}

void LinkingInitFunc::show(std::ostream& os, Module* module)
{
    os << "    function=" << functionIndex << ", priority=" << priority << '\n';
}

LinkingInitFuncSubsection* LinkingInitFuncSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingInitFuncSubsection>();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->inits.emplace_back(LinkingInitFunc::read(context));
    }

    return result;
}

void LinkingInitFuncSubsection::show(std::ostream& os, Module* module)
{
    os << "  init functions\n";

    for (auto& init : inits) {
        init->show(os, module);
    }
}

ComdatSym* ComdatSym::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<ComdatSym>();

    result->kind = ComdatSymKind(data.getU8());
    result->index = data.getU32leb();

    return result;
}

void ComdatSym::show(std::ostream& os, Module* module)
{
    os << "      " << kind << ", index=" << index << '\n';
}

LinkingComdat* LinkingComdat::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingComdat>();

    result->name = readByteArray(context);
    result->flags = data.getU8();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->syms.emplace_back(ComdatSym::read(context));
    }

    return result;
}

void LinkingComdat::show(std::ostream& os, Module* module)
{
    os << "    name =\"" << name << "\", flags=" << flags << "\n";

    for (auto& sym : syms) {
        sym->show(os, module);
    }
}

LinkingComdatSubsection* LinkingComdatSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingComdatSubsection>();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->comdats.emplace_back(LinkingComdat::read(context));
    }

    return result;
}

void LinkingComdatSubsection::show(std::ostream& os, Module* module)
{
    os << "  comdats\n";

    for (auto& comdat : comdats) {
        comdat->show(os, module);
    }
}

SymbolTableFGETInfo* SymbolTableFGETInfo::read(BinaryContext& context,
        SymbolKind kind, SymbolFlags flags)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<SymbolTableFGETInfo>();

    result->index = data.getU32leb();

    if ((flags & SymbolFlagUndefined) == 0 || (flags & SymbolFlagExplicitName) != 0) {
        result->name = readByteArray(context);

        switch(kind) {
            case SymbolKind::function:
                context.getModule()->getFunction(result->index)->setId(result->name);
                break;

            default:
                break;
        }
    }

    return result;
}

void SymbolTableFGETInfo::show(std::ostream& os, Module* module)
{
    os << "    " << kind << ", index=" << index;

    if (!name.empty()) {
        os << ", name=\"" << name << '\"';
    }

    showFlags(os);
    os << '\n';
}

SymbolTableDataInfo* SymbolTableDataInfo::read(BinaryContext& context, SymbolFlags flags)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<SymbolTableDataInfo>();

    result->name = readByteArray(context);

    if ((flags & SymbolFlagUndefined) == 0) {
        result->dataIndex = data.getU32leb();
        result->offset = data.getU32leb();
        result->size = data.getU32leb();
    }

    return result;
}

void SymbolTableDataInfo::show(std::ostream& os, Module* module)
{
    os << "    " << kind << ", name=\"" << name << '\"';

    if ((flags & SymbolFlagUndefined) == 0) {
        os << ", index=" << dataIndex << ", offset-" << offset << ", size=" << size;
    }

    showFlags(os);
    os << '\n';
}

SymbolTableSectionInfo* SymbolTableSectionInfo::read(BinaryContext& context, SymbolFlags flags)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<SymbolTableSectionInfo>();

    result->tableIndex = data.getU32leb();

    return result;
}

void SymbolTableSectionInfo::show(std::ostream& os, Module* module)
{
    os << "    " << kind << ", index=" << tableIndex;
    showFlags(os);
    os << '\n';
}

SymbolTableInfo* SymbolTableInfo::read(BinaryContext& context)
{
    auto& data = context.data();
    auto kind = SymbolKind(data.getU8());
    auto flags = SymbolFlags(data.getU32leb());

    auto result = context.makeTreeNode<SymbolTableInfo>();

    switch (kind) {
        case SymbolKind::function:
        case SymbolKind::global:
        case SymbolKind::event:
        case SymbolKind::table:
            result = SymbolTableFGETInfo::read(context, kind, flags);
            break;

        case SymbolKind::data:
            result = SymbolTableDataInfo::read(context, flags);
            break;

        case SymbolKind::section:
            result = SymbolTableSectionInfo::read(context, flags);
            break;
    }

    result->kind = kind;
    result->flags = flags;

    context.getModule()->addSymbol(result);

    return result;
}

void SymbolTableInfo::show(std::ostream& os, Module* module)
{
}

void SymbolTableInfo::showFlags(std::ostream& os)
{
    if (flags != SymbolFlagNone) {
        if ((flags & SymbolFlagWeak) != 0) {
            os << ", weak";
        }

        if ((flags & SymbolFlagLocal) != 0) {
            os << ", local";
        }

        if ((flags & SymbolFlagHidden) != 0) {
            os << ", hidden";
        }

        if ((flags & SymbolFlagUndefined) != 0) {
            os << ", undefined";
        }

        if ((flags & SymbolFlagExported) != 0) {
            os << ", exported";
        }

        if ((flags & SymbolFlagExplicitName) != 0) {
            os << ", explicitName";
        }

        if ((flags & SymbolFlagNoStrip) != 0) {
            os << ", noStrip";
        }
    }
}

LinkingSymbolTableSubSectionn* LinkingSymbolTableSubSectionn::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingSymbolTableSubSectionn>();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->infos.emplace_back(SymbolTableInfo::read(context));
    }

    return result;
}

void LinkingSymbolTableSubSectionn::show(std::ostream& os, Module* module)
{
    os << "  Symbol table\n";

    for (auto& info : infos) {
        info->show(os, module);
    }
}

LinkingSubsection* LinkingSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto type = LinkingType(data.getU8());
    auto length = data.getU32leb();
    auto startPos = data.getPos();

    auto result = context.makeTreeNode<LinkingSubsection>();

    switch (type) {
        case LinkingType::segmentInfo: 
            result = LinkingSegmentSubsection::read(context);
            break;

        case LinkingType::initFuncs: 
            result = LinkingInitFuncSubsection::read(context);
            break;

        case LinkingType::comDatInfo: 
            result = LinkingComdatSubsection::read(context);
            break;

        case LinkingType::symbolTable: 
            result = LinkingSymbolTableSubSectionn::read(context);
            break;

        default:
            context.msgs().error("Linking type ", type, " not implemented;");
            data.setPos(startPos + length);
    }

    result->type = type;
    return result;
}

void LinkingSubsection::show(std::ostream& os, Module* module)
{
}

LinkingSection* LinkingSection::read(BinaryContext& context, size_t endPos)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<LinkingSection>();

    result->version = data.getU32leb();
    context.msgs().errorWhen(result->version != wasmLinkingVersion,
            "Imvalid linking section version ", result->version);

    while (data.getPos() < endPos) {
        result->subSections.emplace_back(LinkingSubsection::read(context));
    }

    return result;
}

void LinkingSection::check(CheckContext& context)
{
    // nothing to do
}

void LinkingSection::generate(std::ostream& os, Module* module)
{
    // nothing to do
}

void LinkingSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Linking section:\n";

    for (auto& subSection : subSections) {
        subSection->show(os, module);
    }

    os << '\n';
}

bool Signature::operator==(const Signature& other)
{
    if (params.size() != other.params.size() || results.size() != other.results.size()) {
        return false;
    }

    for (size_t i = 0, c = params.size(); i < c; ++i) {
        if (params[i]->getType() != other.params[i]->getType()) {
            return false;
        }
    }

    for (size_t i = 0, c = results.size(); i < c; ++i) {
        if (results[i] != other.results[i]) {
            return false;
        }
    }

    return true;
}

void Signature::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU32leb(uint32_t(params.size()));

    for (const auto& param : params) {
        writeValueType(context, param->getType());
    }

    data.putU32leb(uint32_t(results.size()));

    for (const auto& result : results) {
        writeValueType(context, result);
    }
}

Signature* Signature::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto result = context.makeTreeNode<Signature>();
    bool found = false;

    while (startClause(context, "param")) {
        found = true;
        if (auto id = context.getId()) {
            if (auto value = parseValueType(context)) {
                auto* local = context.makeTreeNode<Local>(*id, *value);

                local->setNumber(module->nextLocalCount());

                result->params.emplace_back(local);

                if (!module->addLocalId(*id, local->getNumber())) {
                    context.msgs().error(tokens.peekToken(-1), "Duplicate local id.");
                }
            }
        } else {
            for (;;) {
                if (auto valueType = parseValueType(context)) {
                    auto* local = context.makeTreeNode<Local>(*valueType);

                    local->setNumber(module->nextLocalCount());

                    result->params.emplace_back(local);
                } else {
                    break;
                }
            }
        }

        requiredCloseParenthesis(context);
    }

    while (startClause(context, "result")) {
        found = true;
        for (;;) {
            if (auto valueType = parseValueType(context)) {
                result->results.push_back(*valueType);
            } else {
                break;
            }
        }

        requiredCloseParenthesis(context);
    }

    if (!found) {
        delete result;
        result = nullptr;
    }

    return result;
}
 
Signature* Signature::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* module = context.getModule();
    auto result = context.makeTreeNode<Signature>();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        auto* local = context.makeTreeNode<Local>(readValueType(context));

        local->setNumber(module->nextLocalCount());
        result->params.emplace_back(local);
    }

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->results.push_back(readValueType(context));
    }

    return result;
}

void Signature::check(CheckContext& context)
{
    for (auto& param : params) {
        param->check(context);
    }

    for (auto result : results) {
        context.checkValueType(this, result);
    }
}

void Signature::generate(std::ostream& os, Module* module)
{
    bool inParam = false;

    for (const auto& param : params) {
        if (!param->getId().empty()) {
            if (inParam) {
                os << ") ";
                inParam = false;
            }

            os << " (param " << param->getId() << ' ' << param->getType() << ')';
        } else {
            if (!inParam) {
                os << " (param";
                inParam = true;
            }

            os << ' ' << param->getType();
        }
    }

    if (inParam) {
        os << ')';
    }

    if (!results.empty()) {
        os << " (result";

        for (const auto& result : results) {
            os << ' ' << result;
        }

        os << ')';
    }
}

void Signature::show(std::ostream& os, Module* module)
{
    const char* separator = "";

    os << " (";
    for (const auto& param : params) {
        os << separator;
        if (!param->getId().empty()) {
            os << param->getId() << ' ';
        }

        os << param->getType();
        separator = ", ";
    }

    os << ") -> ";

    separator = "";
    if (results.empty()) {
        os << "void";
    } else {
        for (const auto& result : results) {
            os << separator << result;
            separator = ", ";
        }
    }
}

void TypeDeclaration::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(func);
    signature->write(context);
}

TypeDeclaration* TypeDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "type")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<TypeDeclaration>();

    result->number = context.getModule()->getTypeCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addTypeId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate type id.");
        }
    }

    if (!requiredStartClause(context, "func")) {
        tokens.recover();
        return result;
    }

    if (auto* sig = Signature::parse(context); sig != nullptr) {
        result->signature.reset(sig);
    } else {
        result->signature.reset(context.makeTreeNode<Signature>());
    }

    // terminate func
    requiredCloseParenthesis(context);

    // terminate type
    requiredCloseParenthesis(context);

    module->endType();

    return result;
}
 
TypeDeclaration* TypeDeclaration::read(BinaryContext& context)
{
    auto result = context.makeTreeNode<TypeDeclaration>();
    auto& data = context.data();

    if (auto f = data.getU8(); f != func) {
        context.msgs().error("Invalid func opcode ", std::hex, f, std::dec);
        return result;
    }

    result->signature.reset(Signature::read(context));
    result->number = context.getModule()->getTypeCount();

    return result;
}

void TypeDeclaration::check(CheckContext& context)
{
    signature->check(context);
}

void TypeDeclaration::generate(std::ostream& os, Module* module)
{
    os << "\n  (type (;" << number << ";) (func";

    signature->generate(os, module);

    os << "))";
}

void TypeDeclaration::generateC(std::ostream& os, const Module* module)
{
    auto& results = signature->getResults();
    auto resultCount = results.size();

    if (!usedAsIndirect) {
        return;
    }

    os << "\ntypedef ";

    if (resultCount == 1) {
        os << results[0].getCName();
    } else {
        os << "void";
    }

    os << "(*" << module->getNamePrefix() << "type" << number << ")(";

    const char* separator = "";

    if (resultCount > 1) {
        for (size_t i = 0; i < resultCount; ++i) {
            auto resultPointerName = makeResultName(0, i) + "_ptr";

            os << separator << results[i].getCName() << " *" << resultPointerName;
            separator = ", ";
        }
    }

    for (auto& param : signature->getParams()) {
        os << separator << param->getType().getCName() << ' ' << param->getCName();
        separator = ", ";
    }

    os << ");";
}

void TypeDeclaration::show(std::ostream& os, Module* module)
{
    os << "  Type " << number << ": ";

    signature->show(os, module);
    os << '\n';
}

void TypeSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::type);
    data.push();
    data.putU32leb(uint32_t(types.size()));

    for (auto& type : types) {
        type->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

TypeSection* TypeSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto* result = new TypeSection;
    auto& msgs = context.msgs();

    msgs.setSectionName("Type");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        auto* typeDeclaration = TypeDeclaration::read(context);

        typeDeclaration->setNumber(i);
        result->types.emplace_back(typeDeclaration);
    }

    if (data.getPos() != startPos + size) {
        msgs.error("Type section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void TypeSection::check(CheckContext& context)
{
    uint32_t count = 0;

    for (auto& type : types) {
        context.msgs().errorWhen(type->getNumber() != count, type.get(),
                "Invalid type number ", type->getNumber(), "; expected ", count);
        ++count;

        type->check(context);
    }
}

void TypeSection::generate(std::ostream& os, Module* module)
{
    for (auto& type : types) {
        type->generate(os, module);
    }
}

void TypeSection::generateC(std::ostream& os, const Module* module)
{
    for (auto& type : types) {
        type->generateC(os, module);
    }
}

void TypeSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Type section:\n";

    for (auto& type : types) {
        type->show(os, module);
    }

    os << '\n';
}

void TypeUse::checkSignature(SourceContext& context)
{
    auto* module = context.getModule();
    auto* typeSection = module->getTypeSection();

    if (signatureIndex == invalidIndex) {
        if (!signature) {
            signature.reset(context.makeTreeNode<Signature>());
        }

        if (typeSection == 0) {
            typeSection = context.makeTreeNode<TypeSection>();

            module->setTypeSectionIndex(module->getSections().size());
            module->getSections().emplace_back(typeSection);
        }

        auto& types = typeSection->getTypes();

        for (uint32_t i = 0, c = uint32_t(types.size()); i < c; ++i) {
            if (*signature == *(types)[i]->getSignature()) {
                signatureIndex = i;
                return;
            }
        }

        auto* typeDeclaration = context.makeTreeNode<TypeDeclaration>(context.makeTreeNode<Signature>(*signature));

        typeDeclaration->setNumber(module->getTypeCount());
        typeSection->getTypes().emplace_back(typeDeclaration);

        signatureIndex = typeDeclaration->getNumber();
    } else {
        auto* typeDeclaration = module->getType(signatureIndex);

        if (signature) {
            if (*signature != *typeDeclaration->getSignature()) {
                context.msgs().error(context.tokens().peekToken(-1), "Signature of function differs from indexed type.");
            }
        } else {
            signature.reset(context.makeTreeNode<Signature>(*typeDeclaration->getSignature()));

            for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
                module->nextLocalCount();
            }
        }
    }
}

void TypeUse::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU32leb(signatureIndex);
}

void TypeUse::parse(SourceContext& context, TypeUse* result, bool forBlock)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (startClause(context, "type")) {
        if (auto id = context.getId()) {
            result->signatureIndex = module->getTypeIndex(*id);
            msgs.errorWhen(result->signatureIndex == invalidIndex, tokens.peekToken(-1),
                    "Type with id '", *id, "' does not exist.");
        } else if (auto index = parseTypeIndex(context)) {
            result->signatureIndex = *index;
            msgs.errorWhen(*index >= module->getTypeCount(), tokens.peekToken(-1),
                    "Type index ", *index, " out of bounds.");
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid type index.");
        }

        requiredCloseParenthesis(context);
    }

    if (auto* sig = Signature::parse(context); sig != nullptr) {
        result->signature.reset(sig);
    } else if (result->signatureIndex != invalidIndex) {
        auto* typeDeclaration = module->getType(result->signatureIndex);

        result->signature.reset(context.makeTreeNode<Signature>(*typeDeclaration->getSignature()));

        for (size_t i = 0, c = result->signature->getParams().size(); i < c; ++i) {
            module->nextLocalCount();
        }
    }

    if (!forBlock) {
        result->checkSignature(context);
    }
}
 
void TypeUse::read(BinaryContext& context, TypeUse* result)
{
    auto& data = context.data();

    result->signatureIndex = data.getU32leb();

    auto* typeDeclaration = context.getModule()->getType(result->signatureIndex);

    result->signature.reset(context.makeTreeNode<Signature>(*typeDeclaration->getSignature()));
}

void TypeUse::generate(std::ostream& os, Module* module)
{
    os << " (type " << signatureIndex << ')';
}

static std::string buildCName(std::string_view id, std::string_view externId, const char* defaultName,
        uint32_t number, bool isExported, const Module* module)
{
    std::string result;
    std::string prefix = module->getNamePrefix();

    if (!externId.empty()) {
        result = cName(externId);
    } else if (!id.empty()) {
        result = prefix + cName(id);
    } else {
        result = prefix + defaultName + toString(number);
    }

    return result;
}

std::string TypeUse::getCName(const Module* module) const
{
    return buildCName(id, externId, "_f_", number, isExported, module);
}

void TypeUse::generateC(std::ostream& os, const Module* module, size_t number)
{
    auto& results = signature->getResults();
    auto resultCount = results.size();

    if (resultCount == 1) {
        os << results[0].getCName();
    } else {
        os << "void";
    }

    os << ' ' << getCName(module) << '(';

    const char* separator = "";

    if (resultCount > 1) {
        for (size_t i = 0; i < resultCount; ++i) {
            auto resultPointerName = makeResultName(0, i) + "_ptr";

            os << separator << results[i].getCName() << " *" << resultPointerName;
            separator = ", ";
        }
    }

    for (auto& param : signature->getParams()) {
        os << separator << param->getType().getCName() << ' ' << param->getCName();
        separator = ", ";
    }

    os << ')';
}

void TypeUse::show(std::ostream& os, Module* module)
{
    os << " signature index=\"" << signatureIndex << "\" ";
    signature->show(os, module);
}

void FunctionImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalType::function));
    TypeUse::write(context);
}

FunctionImport* FunctionImport::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "func")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<FunctionImport>();

    module->addFunction(result);
    module->startFunction();
    result->number = module->nextFunctionCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addFunctionId(result->id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate function id '", result->id, "'.");
        }
    }

    TypeUse::parse(context, result);

    requiredCloseParenthesis(context);

    return result;
}
 
FunctionImport* FunctionImport::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto result = context.makeTreeNode<FunctionImport>();

    module->addFunction(result);
    result->number = module->nextFunctionCount();

    TypeUse::read(context, result);

    return result;
}

void FunctionImport::check(CheckContext& context)
{
    externId = moduleName + "__" + name;
    context.checkTypeIndex(this, signatureIndex);
    signature->check(context);
}

void FunctionImport::generate(std::ostream& os, Module* module)
{
    os << "\n  (import";
    generateNames(os);
    os << " (func (;" << number << ";)";
    static_cast<TypeUse*>(this)->generate(os, module);
    os << "))";
}

void FunctionImport::generateC(std::ostream& os, const Module* module)
{
    os << "\nextern ";
    static_cast<TypeUse*>(this)->generateC(os, module);
    os << ';';
}

void FunctionImport::show(std::ostream& os, Module* module)
{
    os << "  func " << number << ':';
    generateNames(os);
    os << ", ";
    static_cast<TypeUse*>(this)->show(os, module);
    os << '\n';
}

std::string Memory::getCName(const Module* module) const
{
    return buildCName(id, externId, "_memory_", number, isExported, module);
}

void MemoryImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalType::memory));
    writeLimits(context, limits);
}

MemoryImport* MemoryImport::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "memory")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<MemoryImport>();

    module->addMemory(result);
    result->number = module->nextMemoryCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addMemoryId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate memory id.");
        }
    }

    makeExport(context, ExternalType::memory, result->number);

    if (auto limits = requiredLimits(context)) {
        result->limits = *limits;
    } else {
        tokens.recover();
        return result;
    }

    requiredCloseParenthesis(context);

    return result;
}
 
MemoryImport* MemoryImport::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto result = context.makeTreeNode<MemoryImport>();

    result->limits = readLimits(context);
    result->number = module->nextMemoryCount();

    return result;
}

void MemoryImport::check(CheckContext& context)
{
    externId = moduleName + "__" + name;
    context.checkLimits(this, limits);
}

void MemoryImport::generate(std::ostream& os, Module* module)
{
    os << "\n  (import";
    generateNames(os);
    os << " (memory (;" << number << ";)";
    limits.generate(os);
    os << "))";
}

void MemoryImport::generateC(std::ostream& os, const Module* module)
{
    os << "\nextern Memory " << getCName(module) << ';';
}

void MemoryImport::show(std::ostream& os, Module* module)
{
    os << "  memory " << number << ':';
    generateNames(os);
    os << ", ";
    limits.show(os);
    os << '\n';
}

void EventImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalType::event));
    data.putU8(uint8_t(attribute));
    data.putU32leb(typeIndex);
}

EventImport* EventImport::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "event")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<EventImport>();

    module->addEvent(result);
    result->number = module->nextEventCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addEventId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate event id.");
        }
    }

    makeExport(context, ExternalType::event, result->number);

    result->setAttribute(EventType(requiredU8(context)));

    if (auto index = parseTypeIndex(context)) {
        result->typeIndex = *index;
    } else {
        msgs.error(tokens.peekToken(), "Missing or invalid Type index.");
    }

    requiredCloseParenthesis(context);

    return result;
}
 
EventImport* EventImport::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<EventImport>();

    result->attribute = EventType(data.getU8());
    result->typeIndex = data.getU32leb();
    result->number = module->nextEventCount();

    return result;
}

void EventImport::check(CheckContext& context)
{
    externId = moduleName + "__" + name;
    context.checkEventType(this, attribute);
    context.checkTypeIndex(this, typeIndex);
}

void EventImport::generate(std::ostream& os, Module* module)
{
    os << attribute << ' ' << typeIndex;
    os << "\n  (import";
    generateNames(os);
    os << " (event (;" << number << ";) " << attribute << ' ' << typeIndex;
    os << "))";
}

void EventImport::show(std::ostream& os, Module* module)
{
    os << "  event " << number << ':';
    generateNames(os);
    os << " type attribute=\"" << attribute << "\" ";
    os << ",  type index=\"" << typeIndex << "\" ";
    os << '\n';
}

std::string Table::getCName(const Module* module) const
{
    return buildCName(id, externId, "_table_", number, isExported, module);
}

void TableImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalType::table));
    writeValueType(context, type);
    writeLimits(context, limits);
}

TableImport* TableImport::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "table")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<TableImport>();

    module->addTable(result);
    result->number = module->nextTableCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addTableId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate table id.");
        }
    }

    if (auto limits = requiredLimits(context)) {
        result->limits = *limits;
    } else {
        tokens.recover();
        return result;
    }

    if (auto elementType = parseElementType(context)) {
        result->type = *elementType;
    } else {
        msgs.expected(tokens.peekToken(), "ref type");
    }

    requiredCloseParenthesis(context);

    return result;
}
 
TableImport* TableImport::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto result = context.makeTreeNode<TableImport>();

    module->addTable(result);
    result->type = readElementType(context);
    result->limits = readLimits(context);
    result->number = module->nextTableCount();

    return result;
}

void TableImport::check(CheckContext& context)
{
    externId = moduleName + "__" + name;
    context.checkElementType(this, type);
    context.checkLimits(this, limits);
}

void TableImport::generate(std::ostream& os, Module* module)
{
    os << "\n  (import";
    generateNames(os);
    os << " (table (;" << number << ";)";
    limits.generate(os);
    os << ' ' << type << "))";
}

void TableImport::generateC(std::ostream& os, const Module* module)
{
    os << "\nextern Table " << getCName(module) << ';';
}

void TableImport::show(std::ostream& os, Module* module)
{
    os << "  table " << number << ':';
    generateNames(os);
    os << ", " << type << ", ";
    limits.show(os);
    os << '\n';
}

void GlobalImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalType::global));
    writeValueType(context, type);
    data.putU8(uint8_t(mut));
}

GlobalImport* GlobalImport::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "global")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<GlobalImport>();

    module->addGlobal(result);
    result->number = module->nextGlobalCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addGlobalId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate global id.");
        }
    }

    if (startClause(context, "mut")) {
        result->mut = Mut::var;
        if (auto valueType = parseValueType(context)) {
            result->type = *valueType;
            requiredCloseParenthesis(context);
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid value type.");
        }
    } else {
        if (auto valueType = parseValueType(context)) {
            result->type = *valueType;
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid value type.");
        }
    }

    requiredCloseParenthesis(context);

    return result;
}
 
GlobalImport* GlobalImport::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<GlobalImport>();

    result->type = readValueType(context);
    result->mut = Mut(data.getU8());
    result->number = module->nextGlobalCount();
    module->addGlobal(result);

    return result;
}

void GlobalImport::check(CheckContext& context)
{
    externId = moduleName + "__" + name;
    context.checkValueType(this, type);
    context.checkMut(this, mut);
}

void GlobalImport::generateC(std::ostream& os, const Module* module)
{
    os << "\nextern ";
    if (mut == Mut::const_) {
        os << "const ";
    }

    os << type.getCName() << ' ' << getCName(module) << ';';
}

void GlobalImport::generate(std::ostream& os, Module* module)
{
    os << "\n  (import";
    generateNames(os);
    os << " (global (;" << number << ";)";

    if (mut == Mut::var) {
        os << " (mut " << type << ')';
    } else {
        os << ' ' << type;
    }

    os << "))";
}

void GlobalImport::show(std::ostream& os, Module* module)
{
    os << "  global " << number << ':';
    generateNames(os);
    os << ", " << type;
    if (mut == Mut::var) {
        os << " mut";
    }

    os << '\n';
}

void ImportDeclaration::generateNames(std::ostream& os)
{
    os << " \"" << moduleName << "\" \"" << name << "\"";
}

ImportDeclaration* ImportDeclaration::parseFunctionImport(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "func")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = context.getId();

    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<FunctionImport>();

    result->setNumber(module->nextFunctionCount());

    makeExport(context, ExternalType::function, result->getNumber());

    if (!requiredStartClause(context, "import")) {
        tokens.setPos(startPos);
        return result;
    }

    if (id) {
        result->setId(*id);

        if (!module->addFunctionId(*id, result->getNumber())) {
            msgs.error(tokens.peekToken(-1), "Duplicate function id '", *id, "'.");
        }
    }

    module->addFunction(result);
    module->startFunction();

    if (auto value = context.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = context.getString()) {
        result->setName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    requiredCloseParenthesis(context);

    TypeUse::parse(context, result);

    requiredCloseParenthesis(context);

    return result;
}

ImportDeclaration* ImportDeclaration::parseTableImport(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "table")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = context.getId();

    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<TableImport>();

    result->setNumber(module->nextTableCount());

    makeExport(context, ExternalType::table, result->getNumber());

    if (!requiredStartClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    if (id) {
        result->setId(*id);

        if (!module->addTableId(*id, result->getNumber())) {
            msgs.error(tokens.peekToken(-1), "Duplicate table id '", *id, "'.");
        }
    }

    module->addTable(result);

    if (auto value = context.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = context.getString()) {
        result->setName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    requiredCloseParenthesis(context);

    if (auto limits = requiredLimits(context)) {
        result->setLimits(*limits);
    } else {
        tokens.recover();
        return result;
    }

    if (auto elementType = parseElementType(context)) {
        result->setType(*elementType);
    } else {
        msgs.expected(tokens.peekToken(), "funcref");
    }

    requiredCloseParenthesis(context);

    return result;
}

ImportDeclaration* ImportDeclaration::parseMemoryImport(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "memory")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = context.getId();

    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<MemoryImport>();

    result->setNumber(module->nextMemoryCount());

    makeExport(context, ExternalType::memory, result->getNumber());

    if (!requiredStartClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    if (id) {
        result->setId(*id);

        if (!module->addMemoryId(*id, result->getNumber())) {
            msgs.error(tokens.peekToken(-1), "Duplicate memory id '", *id, "'.");
        }
    }

    module->addMemory(result);

    if (auto value = context.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = context.getString()) {
        result->setName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    requiredCloseParenthesis(context);

    if (auto limits = requiredLimits(context)) {
        result->setLimits(*limits);
    } else {
        tokens.recover();
        return result;
    }

    requiredCloseParenthesis(context);

    return result;
}

ImportDeclaration* ImportDeclaration::parseEventImport(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "event")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = context.getId();

    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<EventImport>();

    result->setNumber(module->nextEventCount());

    makeExport(context, ExternalType::event, result->getNumber());

    if (!requiredStartClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    if (id) {
        result->setId(*id);

        if (!module->addEventId(*id, result->getNumber())) {
            msgs.error(tokens.peekToken(-1), "Duplicate event id '", *id, "'.");
        }
    }

    module->addEvent(result);

    if (auto value = context.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = context.getString()) {
        result->setName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    requiredCloseParenthesis(context);

    if (auto index = parseTypeIndex(context)) {
        result->setIndex(*index);
    } else {
        msgs.error(tokens.peekToken(), "Missing or invalid Type index.");
    }

    requiredCloseParenthesis(context);

    return result;
}

ImportDeclaration* ImportDeclaration::parseGlobalImport(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "global")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = context.getId();

    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<GlobalImport>();

    result->setNumber(module->nextGlobalCount());

    makeExport(context, ExternalType::global, result->getNumber());

    if (!requiredStartClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    if (id) {
        result->setId(*id);

        if (!module->addGlobalId(*id, result->getNumber())) {
            msgs.error(tokens.peekToken(-1), "Duplicate global id '", *id, "'.");
        }
    }

    module->addGlobal(result);

    if (auto value = context.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = context.getString()) {
        result->setName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    requiredCloseParenthesis(context);

    if (startClause(context, "mut")) {
        result->setMut(Mut::var);
        if (auto valueType = parseValueType(context)) {
            result->setType(*valueType);
            requiredCloseParenthesis(context);
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid value type.");
        }
    } else {
        if (auto valueType = parseValueType(context)) {
            result->setType(*valueType);
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid value type.");
        }
    }

    requiredCloseParenthesis(context);

    return result;
}


ImportDeclaration* ImportDeclaration::parse(SourceContext& context)
{
    if (auto* result = parseFunctionImport(context); result != nullptr) {
        return result;
    }

    if (auto* result = parseTableImport(context); result != nullptr) {
        return result;
    }

    if (auto* result = parseMemoryImport(context); result != nullptr) {
        return result;
    }

    if (auto* result = parseEventImport(context); result != nullptr) {
        return result;
    }

    if (auto* result = parseGlobalImport(context); result != nullptr) {
        return result;
    }

    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "import")) {
        return nullptr;
    }

    std::string name;
    std::string moduleName;

    if (auto value = context.getString()) {
        moduleName = *value;
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = context.getString()) {
        name = *value;
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    ImportDeclaration* result = nullptr;

    if (auto entry = FunctionImport::parse(context); entry) {
        result = entry;
    } else if (auto entry = TableImport::parse(context); entry) {
        result = entry;
    } else if (auto entry = MemoryImport::parse(context); entry) {
        result = entry;
    } else if (auto entry = EventImport::parse(context); entry) {
        result = entry;
    } else if (auto entry = GlobalImport::parse(context); entry) {
        result = entry;
    } else {
        msgs.expected(tokens.peekToken(), "one of '(memory', '(global', '(func' or '(table'");
        tokens.recover();
        return result;
    }

    result->setModuleName(moduleName);
    result->setName(name);

    requiredCloseParenthesis(context);

    return result;
}
 
void ImportSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::import);
    data.push();
    data.putU32leb(uint32_t(imports.size()));

    for (auto& import : imports) {
        import->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

ImportSection* ImportSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<ImportSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("Import");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        auto moduleName = readByteArray(context);
        auto name = readByteArray(context);

        ImportDeclaration* import = nullptr;
        auto kind = readExternalType(context);

        switch (kind) {
            case ExternalType::function: import = FunctionImport::read(context); break;
            case ExternalType::table:    import = TableImport::read(context); break;
            case ExternalType::memory:   import = MemoryImport::read(context); break;
            case ExternalType::event:   import = EventImport::read(context); break;
            case ExternalType::global:   import = GlobalImport::read(context); break;
            default:
                msgs.error("Invalid import declaration ", uint8_t(kind));
                break;
        }

        if (import == nullptr) {
            break;
        }

        import->setModuleName(moduleName);
        import->setName(name);

        context.getModule()->addImport(import);
        result->imports.emplace_back(import);
    }

    if (data.getPos() != startPos + size) {
        msgs.error("Import section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void ImportSection::check(CheckContext& context)
{
    for (auto& import : imports) {
        import->check(context);
    }
}

void ImportSection::generate(std::ostream& os, Module* module)
{
    for (auto& import : imports) {
        import->generate(os, module);
    }
}

void ImportSection::generateC(std::ostream& os, const Module* module)
{
    for (auto& import : imports) {
        import->generateC(os, module);
    }
}

void ImportSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Import section:\n";

    for (auto& import : imports) {
        import->show(os, module);
    }

    os << '\n';
}

void FunctionDeclaration::write(BinaryContext& context) const
{
    TypeUse::write(context);
}

FunctionDeclaration* FunctionDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (!startClause(context, "func")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<FunctionDeclaration>();

    module->addFunction(result);
    module->startFunction();

    result->number = module->nextFunctionCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addFunctionId(result->id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate function id.");
        }
    }

    makeExport(context, ExternalType::function, result->number);

    TypeUse::parse(context, result);
    module->endFunction();

    // no closing parenthesis because code entry follows.
    return result;
}
 
FunctionDeclaration* FunctionDeclaration::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto result = context.makeTreeNode<FunctionDeclaration>();

    module->addFunction(result);

    TypeUse::read(context, result);
    result->number = module->nextFunctionCount();

    return result;
}

void FunctionDeclaration::check(CheckContext& context)
{
    context.checkTypeIndex(this, signatureIndex);
    signature->check(context);
}

void FunctionDeclaration::generate(std::ostream& os, Module* module)
{
    // nothing to do
}

void FunctionDeclaration::generateC(std::ostream& os, const Module* module)
{
    os << '\n';

    if (!isExported) {
        os << "static ";
    }

    static_cast<TypeUse*>(this)->generateC(os, module, number);
    os << ';';
}

void FunctionDeclaration::show(std::ostream& os, Module* module)
{
    os << "  func " << number;

    if (!id.empty()) {
        os << " \"" << id << "\"";
    }

    os << ": ";
    static_cast<TypeUse*>(this)->show(os, module);
    os << '\n';
}

void FunctionSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::function);
    data.push();
    data.putU32leb(uint32_t(functions.size()));

    for (auto& function : functions) {
        function->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

FunctionSection* FunctionSection::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<FunctionSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("Function");

    result->setData(context, startPos, startPos + size);

    module->startLocalFunctions();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->functions.emplace_back(FunctionDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error("Function section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void FunctionSection::check(CheckContext& context)
{
    uint32_t count = context.getModule()->getImportedFunctionCount();

    for (auto& function : functions) {
        context.msgs().errorWhen(function->getNumber() != count, function.get(),
                "Invalid function number ", function->getNumber(), "; exported ", count);
        ++count;

        function->check(context);
    }
}

void FunctionSection::generate(std::ostream& os, Module* module)
{
    // nothing to do
}

void FunctionSection::generateC(std::ostream& os, const Module* module)
{
    for (auto& function : functions) {
        function->generateC(os, module);
    }
}

void FunctionSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Function section:\n";

    for (auto& function : functions) {
        function->show(os, module);
    }

    os << '\n';
}

void TableDeclaration::write(BinaryContext& context) const
{
    writeValueType(context, type);
    writeLimits(context, limits);
}

TableDeclaration* TableDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "table")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<TableDeclaration>();

    module->addTable(result);
    result->number = module->nextTableCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addTableId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate table id.");
        }
    }

    makeExport(context, ExternalType::table, result->number);

    if (auto elementType = parseElementType(context)) {
        result->type = *elementType;

        if (startClause(context, "elem")) {
            auto element = context.makeTreeNode<ElementDeclaration>();

            element->setNumber(module->nextElementCount());
            element->setTableIndex(result->number);

            uint32_t functionIndexCount = element->parseFunctionIndexExpressions(context); 

            if (functionIndexCount == 0) {
                functionIndexCount = element->parseFunctionIndexes(context); 
            }

            if (functionIndexCount == 0) {
                msgs.error(tokens.peekToken(), "Missing or invalid function index.");
            }

            element->setExpression(makeI32Const0(context));

            requiredCloseParenthesis(context);

            module->addElementEntry(element);

            result->limits = Limits(functionIndexCount, functionIndexCount);
        } else {
            msgs.expected(tokens.peekToken(), "(elem");
            tokens.recover();
        }
    } else if (auto limits = requiredLimits(context)) {
        result->limits = *limits;

        if (auto elementType = parseElementType(context)) {
            result->type = *elementType;
        } else {
            msgs.expected(tokens.peekToken(), "funcref");
        }
    } else {
        tokens.recover();
        return result;
    }

    requiredCloseParenthesis(context);

    return result;
}
 
TableDeclaration* TableDeclaration::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto result = context.makeTreeNode<TableDeclaration>();

    module->addTable(result);
    result->type = readElementType(context);

    result->limits = readLimits(context);
    result->number = module->nextTableCount();

    return result;
}

void TableDeclaration::show(std::ostream& os, Module* module)
{
    os << "  table ";
    shsowFunctionIndex(os, number, module);

    os << ": type=" << type << ',';
    limits.show(os);
    os << '\n';
}

void TableDeclaration::check(CheckContext& context)
{
    context.checkElementType(this, type);
    context.checkLimits(this, limits);
}

void TableDeclaration::generate(std::ostream& os, Module* module)
{
    os << "\n  (table (;" << number << ";)";
    limits.generate(os);
    os << ' ' << type;

    os << ')';
}

void TableDeclaration::generateC(std::ostream& os, const Module* module)
{
    os << "\nTable " << getCName(module) << ';';
}

void TableSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::table);
    data.push();
    data.putU32leb(uint32_t(tables.size()));

    for (auto& table : tables) {
        table->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

TableSection* TableSection::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<TableSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    module->startLocalTables();

    msgs.setSectionName("Table");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->tables.emplace_back(TableDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error("Table section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void TableSection::check(CheckContext& context)
{
    for (auto& table : tables) {
        table->check(context);
    }
}

void TableSection::generate(std::ostream& os, Module* module)
{
    for (auto& table : tables) {
        table->generate(os, module);
    }
}

void TableSection::generateC(std::ostream& os, const Module* module)
{
    for (auto& table : tables) {
        table->generateC(os, module);
    }
}

void TableSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Table section:\n";

    for (auto& table : tables) {
        table->show(os, module);
    }

    os << '\n';
}

void MemoryDeclaration::write(BinaryContext& context) const
{
    writeLimits(context, limits);
}

MemoryDeclaration* MemoryDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "memory")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<MemoryDeclaration>();
    result->number = module->nextMemoryCount();
    module->addMemory(result);

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addMemoryId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate memory id.");
        }
    }

    makeExport(context, ExternalType::memory, result->number);

    if (startClause(context, "data")) {
        auto dataEntry = context.makeTreeNode<DataSegment>();

        dataEntry->setNumber(module->getSegmentCount());

        std::string init;

        while (auto str = context.getString()) {
            init.append(*str);
        }

        uint32_t m = uint32_t((init.size() + 0x10000 - 1) / 0x10000);

        result->setLimits({ m, m });

        dataEntry->setInit(init);
        dataEntry->setExpression(makeI32Const0(context));

        module->addDataEntry(dataEntry);

        requiredCloseParenthesis(context);
    } else {
        if (auto limits = requiredLimits(context)) {
            result->limits = *limits;
        } else {
            tokens.recover();
        }
    }

    requiredCloseParenthesis(context);

    return result;
}
 
MemoryDeclaration* MemoryDeclaration::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto result = context.makeTreeNode<MemoryDeclaration>();

    module->addMemory(result);
    result->limits = readLimits(context);
    result->number = module->nextMemoryCount();

    return result;
}

void MemoryDeclaration::show(std::ostream& os, Module* module)
{
    os << "  memory ";
    shsowFunctionIndex(os, number, module);

    os << ':';
    limits.show(os);
    os << '\n';
}

void MemoryDeclaration::check(CheckContext& context)
{
    context.checkLimits(this, limits);
}

void MemoryDeclaration::generate(std::ostream& os, Module* module)
{
    os << "\n  (memory (;" << number << ";)";
    limits.generate(os);

    os << ')';
}

void MemoryDeclaration::generateC(std::ostream& os, const Module* module)
{
    os << "\nMemory " << getCName(module) << ';';
}

void MemorySection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::memory);
    data.push();
    data.putU32leb(uint32_t(memories.size()));

    for (auto& memory : memories) {
        memory->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

MemorySection* MemorySection::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<MemorySection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    module->startLocalMemories();

    msgs.setSectionName("Memory");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->memories.emplace_back(MemoryDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error(" memory section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void MemorySection::check(CheckContext& context)
{
    for (auto& memory : memories) {
        memory->check(context);
    }
}

void MemorySection::generate(std::ostream& os, Module* module)
{
    for (auto& memory : memories) {
        memory->generate(os, module);
    }
}

void MemorySection::generateC(std::ostream& os, const Module* module)
{
    for (auto& memory : memories) {
        memory->generateC(os, module);
    }
}

void MemorySection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Memory section:\n";

    for (auto& memory : memories) {
        memory->show(os, module);
    }

    os << '\n';
}

void GlobalDeclaration::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeValueType(context, type);
    data.putU8(uint8_t(mut));
    expression->write(context);
}

GlobalDeclaration* GlobalDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "global")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<GlobalDeclaration>();

    result->number = module->nextGlobalCount();
    module->addGlobal(result);

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addGlobalId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate global id.");
        }
    }

    makeExport(context, ExternalType::global, result->number);

    if (startClause(context, "mut")) {
        result->mut = Mut::var;
        if (auto valueType = parseValueType(context)) {
            result->type = *valueType;
            requiredCloseParenthesis(context);
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid value type.");
        }
    } else {
        if (auto valueType = parseValueType(context)) {
            result->type = *valueType;
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid value type.");
        }
    }

    if (!requiredParenthesis(context, '(')) {
        tokens.recover();
        return result;
    }

    result->expression.reset(requiredExpression(context));

    requiredCloseParenthesis(context);

    requiredCloseParenthesis(context);

    return result;
}
 
GlobalDeclaration* GlobalDeclaration::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<GlobalDeclaration>();

    result->type = readValueType(context);
    result->mut = Mut(data.getU8());
    result->number = module->nextGlobalCount();
    result->expression.reset(Expression::readInit(context));
    module->addGlobal(result);

    return result;
}

void GlobalDeclaration::show(std::ostream& os, Module* module)
{
    os << "  global " << number << ": ";
    if (mut == Mut::var) {
        os << "mut ";
    }

    expression->show(os, module);
    os << '\n';

}

void GlobalDeclaration::check(CheckContext& context)
{
    context.checkValueType(this, type);
    context.checkMut(this, mut);
    expression->check(context);
    context.checkInitExpression(expression.get(), type);
}

void GlobalDeclaration::generate(std::ostream& os, Module* module)
{
    os << "\n  (global (;" << number << ";)";

    if (mut == Mut::var) {
        os << " (mut " << type << ')';
    } else {
        os << ' ' << type;
    }

    os << " (";
    expression->generate(os, module);
    os << ')';

    os << ')';
}

std::string Global::getCName(const Module* module) const
{
    return buildCName(id, externId, "_global_", number, isExported, module);
}

void GlobalDeclaration::generateC(std::ostream& os, const Module* module)
{
    os << '\n';

    if (!isExported) {
        os << "static ";
    }

    if (mut == Mut::const_) {
        os << "const ";
    }

    os << type.getCName() << ' ' << getCName(module);

    if (expression) {
        os << " = ";
        expression->generateCValue(os, module);
    }

    os << ';';
}

void GlobalSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::global);
    data.push();
    data.putU32leb(uint32_t(globals.size()));

    for (auto& global : globals) {
        global->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

GlobalSection* GlobalSection::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<GlobalSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    module->startLocalGlobals();

    msgs.setSectionName("Global");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->globals.emplace_back(GlobalDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error("Global section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void GlobalSection::check(CheckContext& context)
{
    for (auto& global : globals) {
        global->check(context);
    }
}

void GlobalSection::generate(std::ostream& os, Module* module)
{
    for (auto& global : globals) {
        global->generate(os, module);
    }
}

void GlobalSection::generateC(std::ostream& os, const Module* module)
{
    for (auto& global : globals) {
        global->generateC(os, module);
    }
}

void GlobalSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Global section:\n";

    for (auto& global : globals) {
        global->show(os, module);
    }

    os << '\n';
}

void ExportDeclaration::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, name);
    data.putU8(uint8_t(kind));
    data.putU32leb(index);
}

ExportDeclaration* ExportDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "export")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<ExportDeclaration>();

    result->number = module->nextExportCount();
    result->name = requiredString(context);

    if (!requiredParenthesis(context, '(')) {
        tokens.recover();
        return result;
    }

    if (auto kind = parseExternalType(context)) {
        result->kind = *kind;
        switch(uint8_t(*kind)) {
            case ExternalType::function:
                if (auto index = parseFunctionIndex(context)) {
                    result->index = *index;
                } else {
                    msgs.error(tokens.peekToken(), "Missing or invalid function index.");
                }

                break;

            case ExternalType::table:
                if (auto index = parseTableIndex(context)) {
                    result->index = *index;
                } else {
                    msgs.error(tokens.peekToken(), "Missing or invalid Table index.");
                }

                break;

            case ExternalType::memory:
                if (auto index = parseMemoryIndex(context)) {
                    result->index = *index;
                } else {
                    msgs.error(tokens.peekToken(), "Missing or invalid memory index.");
                }

                break;

            case ExternalType::global:
                if (auto index = parseGlobalIndex(context)) {
                    result->index = *index;
                } else {
                    msgs.error(tokens.peekToken(), "Missing or invalid global index.");
                }

                break;

            default:
                break;
        }

    } else {
        msgs.error(tokens.peekToken(), "Missing or invalid export kind.");
    }

    requiredCloseParenthesis(context);

    requiredCloseParenthesis(context);

    return result;
}
 
ExportDeclaration* ExportDeclaration::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<ExportDeclaration>();

    result->name = readByteArray(context);
    result->kind = readExternalType(context);
    result->index = data.getU32leb();
    result->number = module->nextExportCount();

    return result;
}

void ExportDeclaration::show(std::ostream& os, Module* module)
{
    os << "  export " << number << ": name=\"" << name << "\", kind=" << kind << ", index=" << index;

    os << '\n';

}

void ExportDeclaration::check(CheckContext& context)
{
    context.checkExternalType(this, uint8_t(kind));

    switch(kind) {
        case ExternalType::function:
            if (auto* function = context.getModule()->getFunction(index); function != nullptr) {
                function->setExported();
            }

            context.checkFunctionIndex(this, index);
            break;

        case ExternalType::table:
            if (auto* table = context.getModule()->getTable(index); table != nullptr) {
                table->setExported();
            }

            context.checkTableIndex(this, index);
            break;

        case ExternalType::memory:
            if (auto* memory = context.getModule()->getMemory(index); memory != nullptr) {
                memory->setExported();
            }

            context.checkMemoryIndex(this, index);
            break;

        case ExternalType::global:
            if (auto* global = context.getModule()->getGlobal(index); global != nullptr) {
                global->setExported();
            }

            context.checkGlobalIndex(this, index);
            break;

        case ExternalType::event:
            if (auto* event = context.getModule()->getEvent(index); event != nullptr) {
                event->setExported();
            }

            context.checkEventIndex(this, index);
            break;

        default:
            break;
    }
}

void ExportDeclaration::generate(std::ostream& os, Module* module)
{
    os << "\n  (export (;" << number << ";) \"" << name << "\" (" << kind << ' ' << index << ')';

    os << ')';
}

void ExportSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::export_);
    data.push();
    data.putU32leb(uint32_t(exports.size()));

    for (auto& export_ : exports) {
        export_->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

ExportSection* ExportSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<ExportSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("Export");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->exports.emplace_back(ExportDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error("Export section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void ExportSection::check(CheckContext& context)
{
    for (auto& export_ : exports) {
        export_->check(context);
    }
}

void ExportSection::generate(std::ostream& os, Module* module)
{
    for (auto& export_ : exports) {
        export_->generate(os, module);
    }
}

void ExportSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Export section:\n";

    for (auto& export_ : exports) {
        export_->show(os, module);
    }

    os << '\n';
}

StartSection* StartSection::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "start")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<StartSection>();

    if (auto index = parseFunctionIndex(context)) {
        result->functionIndex = *index;
    } else {
        context.msgs().error(tokens.peekToken(), "Missing or invalid function index.");
    }

    requiredCloseParenthesis(context);

    return result;
}

StartSection* StartSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<StartSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("Start");

    result->setData(context, startPos, startPos + size);

    result->functionIndex = data.getU32leb();

    if (data.getPos() != startPos + size) { 
        msgs.error("Start section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void StartSection::check(CheckContext& context)
{
    context.checkFunctionIndex(this, functionIndex);
}

void StartSection::generate(std::ostream& os, Module* module)
{
    os << "\n  (start " << functionIndex << ')';
}

void StartSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Start section:\n";

    os << "    start=" << functionIndex << '\n';

    os << '\n';
}

void StartSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::start);
    data.push();
    data.putU32leb(functionIndex);

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

void Expression::write(BinaryContext& context) const
{
    for (auto& instruction : instructions) {
        instruction->write(context);
    }

    context.data().putU8(Opcode::end);
}

Expression* Expression::parse(SourceContext& context, bool oneInstruction)
{
    auto result = context.makeTreeNode<Expression>();

    if (oneInstruction) {
        if (auto* instruction = Instruction::parse(context)) {
            result->instructions.emplace_back(instruction);
        }
    } else {
        std::vector<Instruction*> instructions;

        Instruction::parse(context, instructions);

        for (auto* instruction : instructions) {
            result->instructions.emplace_back(instruction);
        }
    }

    if (result->instructions.empty()) {
        delete result;
        return nullptr;
    }

    return result;
}
 
Expression* Expression::read(BinaryContext& context)
{
    auto result = context.makeTreeNode<Expression>();

    for (;;) {
        auto instruction = Instruction::read(context);
        bool end = instruction->getOpcode() == Opcode::end;

        result->instructions.emplace_back(instruction);

        if (end) {
            break;
        }
    }

    return result;
}

Expression* Expression::read(BinaryContext& context, size_t endPos)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<Expression>();

    while (data.getPos() < endPos) {
        result->instructions.emplace_back(Instruction::read(context));
    }

    return result;
}

Expression* Expression::readInit(BinaryContext& context)
{
    auto result = context.makeTreeNode<Expression>();

    result->instructions.emplace_back(Instruction::read(context));

    auto instruction = Instruction::read(context);
    context.msgs().errorWhen(instruction->getOpcode() != Opcode::end,
            "Init expression must be terminated with 'end' instruction");

    return result;
}

void Expression::check(CheckContext& context)
{
    for (auto& instruction : instructions) {
        instruction->check(context);
    }
}

void Expression::generate(std::ostream& os, Module* module)
{
    const char* separator = "";
    InstructionContext instructionContext(module);
    auto count = getInstructions().size();

    for (auto& instruction : instructions) {
        if (--count == 0 && instruction->getOpcode() == Opcode::end) {
            break;
        }

        os << separator;
        instruction->generate(os, instructionContext);
        separator = "; ";
    }
}

void Expression::generateCValue(std::ostream& os, const Module* module)
{
    assert(instructions.size() == 1);

    auto* instruction = instructions[0].get();

    switch(instruction->getOpcode()) {
        case Opcode::i32__const:
            os << static_cast<InstructionI32*>(instruction)->getValue();
            return;

        case Opcode::i64__const:
            os << static_cast<InstructionI64*>(instruction)->getValue();
            return;

        case Opcode::f32__const:
            os << static_cast<InstructionF32*>(instruction)->getValue();
            return;

        case Opcode::f64__const:
            os << static_cast<InstructionF64*>(instruction)->getValue();
            return;

        case Opcode::v128__const:
            {
                union
                {
                    int64_t a64[2];
                    v128_t v128;
                };

                v128 = static_cast<InstructionV128*>(instruction)->getValue();

                os << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << a64[0] <<
                    "LL, 0x" << std::setw(16) << std::setfill('0') << a64[0] << std::dec << "LL }";
            }

            break;

        case Opcode::global__get:
            {
                auto globalIndex = static_cast<InstructionGlobalIdx*>(instruction)->getIndex();
                auto* global = module->getGlobal(globalIndex);

                os << global->getCName(module);
            }

            break;

        case Opcode::ref__null:
            os << "NULL";
            break;

        case Opcode::ref__func:
            {
                auto functionIndex = static_cast<InstructionFunctionIdx*>(instruction)->getIndex();
                auto* function = module->getFunction(functionIndex);

                os << "(void*)" << function->getCName(module);
            }

            break;

        default:
            // TBI
            assert(false);

    }
}


void Expression::show(std::ostream& os, Module* module)
{
    generate(os, module);
}

void ElementDeclaration::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU32leb(flags);

    if ((flags & SegmentFlagDeclared) == SegmentFlagExplicitIndex) {
        data.putU32leb(tableIndex);
    }

    if ((flags & SegmentFlagPassive) == 0) {
        expression->write(context);
    }

    if ((flags & SegmentFlagDeclared) != 0) {
        if ((flags & SegmentFlagElemExpr) != 0) {
            writeValueType(context, elementType);
        } else {
            data.putU8(ExternalType::function);
        }
    }

    if (flags & SegmentFlagElemExpr) {
        data.putU32leb(uint32_t(refExpressions.size()));

        for (auto& edxpression : refExpressions) {
            edxpression->write(context);
        }
    } else {
        data.putU32leb(uint32_t(functionIndexes.size()));

        for (auto f : functionIndexes) {
            data.putU32leb(f);
        }
    }
}

uint32_t ElementDeclaration::parseFunctionIndexExpressions(SourceContext context)
{
    auto& tokens = context.tokens();
    uint32_t result = 0;

    while (tokens.getParenthesis('(')) {
        ++result;

        bool extraParenthesis = false;

        if (tokens.getKeyword("item")) {
            extraParenthesis = tokens.getParenthesis('(');
        }

        if (auto* expression = requiredExpression(context, true); expression != nullptr) {
            refExpressions.emplace_back(expression);

            if ((flags & SegmentFlagElemExpr) == 0) {
                auto* instruction = refExpressions.back()->getInstructions()[0].get();
                auto opcode = instruction->getOpcode();

                if (opcode == Opcode::ref__null) {
                    flags = SegmentFlags(flags | SegmentFlagElemExpr);
                } else if (opcode == Opcode::ref__func) {
                    auto *f = static_cast<InstructionFunctionIdx*>(instruction);

                    functionIndexes.push_back(f->getIndex());
                }
            }
        }

        if (extraParenthesis) {
            requiredCloseParenthesis(context);
        }

        requiredCloseParenthesis(context);
    }

    return result;
}

uint32_t ElementDeclaration::parseFunctionIndexes(SourceContext context)
{
    auto& tokens = context.tokens();
    uint32_t result = 0;

    (void) tokens.getKeyword("func");

    while (auto index = parseFunctionIndex(context)) {
        ++result;
        functionIndexes.push_back(*index);
    }

    return result;
}

ElementDeclaration* ElementDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "elem")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<ElementDeclaration>();

    result->number = module->nextElementCount();

    if (auto id = context.getId()) {
        if (auto index = module->getTableIndex(*id); index != invalidIndex) {
            result->tableIndex = index;
        } else {
            result->id = *id;

            if (!module->addElementId(*id, result->number)) {
                msgs.error(tokens.peekToken(-1), "Duplicate element id.");
            }
        }
    }

    if (auto index = parseTableIndex(context)) {
        result->tableIndex = *index;
    } else if (startClause(context, "table")) {
        if (auto index = parseTableIndex(context)) {
            result->tableIndex = *index;
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid table index.");
        }

        requiredCloseParenthesis(context);
    }


    if (result->tableIndex != 0) {
        result->flags = SegmentFlags(result->flags | SegmentFlagExplicitIndex);
    }

    if (tokens.getKeyword("declare")) {
        result->flags = SegmentFlagDeclared;
    }

    if (result->flags != SegmentFlagDeclared) {
        if (startClause(context, "offset")) {
            result->expression.reset(requiredExpression(context));

            requiredCloseParenthesis(context);
        } else if (tokens.getParenthesis('(')) {
            result->expression.reset(requiredExpression(context, true));

            requiredCloseParenthesis(context);
        } else {
            result->flags = SegmentFlagPassive;
        }
    }

    if (auto type = parseValueType(context)) {
        if (*type == ValueType::func) {
            result->elementType = ValueType::funcref;
            result->parseFunctionIndexes(context);
        } else {
            result->elementType = *type;

            result->parseFunctionIndexExpressions(context);
        }
    } else {
        result->elementType = ValueType::funcref;
        result->parseFunctionIndexes(context);
    }

    requiredCloseParenthesis(context);

    return result;
}
 
ElementDeclaration* ElementDeclaration::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<ElementDeclaration>();

    result->number = module->nextElementCount();

    result->flags = SegmentFlags(data.getU32leb());
    msgs.errorWhen((result->flags > SegmentFlagMax), "Invalid segment flags.");

    if ((result->flags & SegmentFlagDeclared) == SegmentFlagExplicitIndex) {
        result->tableIndex = data.getU32leb();
    }

    result->elementType = ValueType::funcref;

    if ((result->flags & SegmentFlagPassive) == 0) {
        result->expression.reset(Expression::readInit(context));
    }

    if ((result->flags & SegmentFlagDeclared) != 0) {
        if ((result->flags & SegmentFlagElemExpr) != 0) {
            result->elementType = readValueType(context);
            msgs.errorWhen((!result->elementType.isValidRef()), "Element type must be a referncetype.");
        } else {
            auto kind = readExternalType(context);

            msgs.errorWhen((kind != ExternalType::function), "Segement element type must be 'func'.");
        }
    }

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        if ((result->flags & SegmentFlagElemExpr) != 0) {
            result->refExpressions.emplace_back(Expression::readInit(context));
        } else {
            result->functionIndexes.push_back(data.getU32leb());
        }
    }

    return result;
}

void ElementDeclaration::check(CheckContext& context)
{
    context.msgs().errorWhen((flags > SegmentFlagMax), this, "Invalid segment flags.");

    if ((flags & SegmentFlagPassive) == 0) {
        context.checkTableIndex(this,tableIndex);
    }

    if (elementType != 0) {
        context.checkValueType(this, elementType);
    }

    if (expression) {
        expression->check(context);
        context.checkInitExpression(expression.get(), ValueType::i32);
    }

    for (auto& expression : refExpressions) {
        expression->check(context);
        context.checkInitExpression(expression.get(), ValueType::nullref);
    }

    for (auto index : functionIndexes) {
        context.checkFunctionIndex(this, index);
    }
}

void ElementDeclaration::generate(std::ostream& os, Module* module)
{
    os << "\n  (elem (;" << number << ";)";

    if ((flags & (SegmentFlagPassive | SegmentFlagExplicitIndex)) == SegmentFlagExplicitIndex) {
        os << ' ' << tableIndex;
    }

    if ((flags & SegmentFlagPassive) == 0) {
        os << " (";
        expression->generate(os, module);
        os << ')';
    }

    if ((flags & (SegmentFlagPassive | SegmentFlagExplicitIndex)) != 0) {
        if ((flags & SegmentFlagElemExpr) != 0) {
            os << " " << elementType;
        }
    }

    if (flags & SegmentFlagElemExpr) {
        os << ' ' << elementType;

        for (auto& ref : refExpressions) {
            os << " (";
            ref->generate(os, module);
            os << ')';
        }
    } else {
        os << " func";
        for (auto func : functionIndexes) {
            os << ' ' << func;
        }
    }

    os << ')';
}

void ElementDeclaration::show(std::ostream& os, Module* module)
{
    os << "  segment " << number << ": " << "table=" << tableIndex;

    if ((flags & SegmentFlagPassive) == 0) {
        os << ", offset=(";
        expression->generate(os, module);
    }

    if ((flags & (SegmentFlagPassive | SegmentFlagExplicitIndex)) != 0) {
        os << ", element type=" << elementType;
    }

    os << "), funcs=[";

    const char* separator = "";

    if (flags & SegmentFlagElemExpr) {
        for (auto& ref : refExpressions) {
            os << separator;
            ref->show(os, module);
            separator = ", ";
        }
    } else {
        for (auto func : functionIndexes) {
            os << separator << func;
            separator = ", ";
        }
    }

    os << "]\n";
}

std::string ElementDeclaration::getCName(const Module* module) const
{
    std::string prefix = module->getNamePrefix();

    return prefix + "_elem_" + toString(number);

}

void ElementSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::element);
    data.push();
    data.putU32leb(uint32_t(elements.size()));

    for (auto& element : elements) {
        element->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

ElementSection* ElementSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<ElementSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("Element");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->elements.emplace_back(ElementDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error("Element section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void ElementSection::check(CheckContext& context)
{
    for (auto& element : elements) {
        element->check(context);
    }
}

void ElementSection::generate(std::ostream& os, Module* module)
{
    for (auto& element : elements) {
        element->generate(os, module);
    }
}

void ElementSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Element section:\n";

    for (auto& element : elements) {
        element->show(os, module);
    }

    os << '\n';
}

std::string Local::getCName() const
{
    std::string result;

    if (!id.empty()) {
        result = cName(id);
    } else {
        result = "_local_" + toString(number);
    }

    return result;
}

Local* Local::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<Local>();

    result->number = module->nextLocalCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addLocalId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate local id.");
        }
    }

    if (auto type = parseValueType(context)) {
        result->type = *type;
    } else {
        msgs.expected(tokens.peekToken(), "Value type");
        tokens.recover();
    }

    return result;
}

void Local::check(CheckContext& context)
{
    context.checkValueType(this, type);
}

void Local::generate(std::ostream& os, Module* module)
{
    os << ' ' << type;
}

void Local::show(std::ostream& os, Module* module)
{
    os << "    local: " << "type=" << type << '\n';
}

void CodeEntry::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.push();

    if (!locals.empty()) {
        auto type = locals[0]->getType();
        uint32_t count = 1;

        for (auto& local : locals) {
            if (local->getType() != type) {
                ++count;
                type = local->getType();
            }
        }

        data.putU32leb(count);

        type = locals[0]->getType();
        count = 0;

        for (auto& local : locals) {
            if (local->getType() == type) {
                ++count;
            } else {
                data.putU32leb(count);
                writeValueType(context, type);
                type = local->getType();
                count = 1;
            }
        }

        data.putU32leb(count);
        writeValueType(context, type);
    } else {
        data.putU32leb(0u);
    }

    expression->write(context);

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

CodeEntry* CodeEntry::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto result = context.makeTreeNode<CodeEntry>();

    result->number = module->nextCodeCount();

    module->startCode(result->number - module->getImportedFunctionCount());

    while (startClause(context, "local")) {
        while (!tokens.peekParenthesis(')')) {
            result->locals.emplace_back(Local::parse(context));
        }

        requiredCloseParenthesis(context);
    }

    auto* expression = Expression::parse(context);

    if (expression == nullptr) {
        expression = context.makeTreeNode<Expression>();
    }

    result->expression.reset(expression);

    // close the '(func' clause.
    requiredCloseParenthesis(context);

    return result;
}
 
CodeEntry* CodeEntry::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<CodeEntry>();

    auto size = data.getU32leb();
    auto startPos = data.getPos();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        auto localCount = data.getU32leb();
        auto type = readValueType(context);

        for (uint32_t j = 0; j < localCount; ++j) {
            auto* local = context.makeTreeNode<Local>(type);

            local->setNumber(module->nextLocalCount());
            result->locals.emplace_back(local);
        }
    }

    result->expression.reset(Expression::read(context, startPos + size));
    result->number = module->nextCodeCount();

    return result;
}

void CodeEntry::check(CheckContext& context)
{
    for (auto& local : locals) {
        local->check(context);
    }

    expression->check(context);
}

void CodeEntry::generate(std::ostream& os, Module* module)
{
    auto* function = module->getFunction(number);
    auto signatureIndex = function->getSignatureIndex();

    os << "\n  (func ";
    if (!function->getId().empty()) {
        os << '$' << function->getId();
    }

    os << "(;" << number;

    if (!function->getExternId().empty()) {
        os << ' ' << function->getExternId();
    }

    os << ";) (type " << signatureIndex << ')';

    function->getSignature()->generate(os, module);

    if (!locals.empty()) {
        os << "\n    (local";

        for (auto& local : locals) {
            local->generate(os, module);
        }

        os << ')';
    }

    if (module->getUseExpressionS()) {
        ExpressionSBuilder builder(module);

        builder.generate(os, this);
    } else {
        auto count = expression->getInstructions().size();
        InstructionContext instructionContext(module);

        for (auto& instruction : expression->getInstructions()) {
            if (--count == 0 && instruction->getOpcode() == Opcode::end) {
                break;
            }

            os << "\n    " << instructionContext.getIndent();

            instruction->generate(os, instructionContext);
        }
    }

    os << ")";
}

void CodeEntry::generateC(std::ostream& os, const Module* module, bool enhanced)
{
    auto* function = module->getFunction(number);

    os << '\n';

    if (!function->getExported()) {
        os << "static ";
    }

    static_cast<TypeUse*>(function)->generateC(os, module, number);

    os << "\n{";

    CGenerator generator(module, this, enhanced);

    generator.generateC(os);

    os << "\n}"
        "\n";
}

void CodeEntry::show(std::ostream& os, Module* module)
{
    for (auto& local : locals) {
        local->show(os, module);
    }

    std::string indent = "";
    InstructionContext instructionContext(module);

    for (auto& instruction : expression->getInstructions()) {
        if (indent.size() > 1 && instruction->getOpcode() == Opcode::end) {
            indent.resize(indent.size() - 2);
        }

        os << "    " << indent;

        instruction->generate(os, instructionContext);

        if (instruction->getImmediateType() == ImmediateType::block) {
            indent.append("  ");
        }

        os << '\n';
    }

    os << '\n';
}

void CodeSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::code);
    data.push();
    data.putU32leb(uint32_t(codes.size()));

    for (auto& code : codes) {
        code->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

CodeSection* CodeSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<CodeSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("Code");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->codes.emplace_back(CodeEntry::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error("Code section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void CodeSection::check(CheckContext& context)
{
    for (auto& code : codes) {
        code->check(context);
    }
}

void CodeSection::generate(std::ostream& os, Module* module)
{
    for (auto& code : codes) {
        code->generate(os, module);
    }
}

void CodeSection::generateC(std::ostream& os, const Module* module, bool enhanced)
{
    for (auto& code : codes) {
        code->generateC(os, module, enhanced);
    }
}

void CodeSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Code section:\n";

    auto count = module->getImportedFunctionCount();

    for (auto& code : codes) {
        os << "  Code for function ";
        shsowFunctionIndex(os, count++, module);

        if ((flags & 1) != 0) {
            os <<  ":\n";
            code->show(os, module);
        } else {
            os << '\n';
        }
    }

    os << '\n';
}

void DataSegment::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU32leb(flags);

    if ((flags & SegmentFlagExplicitIndex) != 0) {
        data.putU32leb(memoryIndex);
    }

    if ((flags & SegmentFlagPassive) == 0) {
        expression->write(context);
    }

    writeByteArray(context, init);
}

DataSegment* DataSegment::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "data")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<DataSegment>();

    result->number = module->getSegmentCount();

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addSegmentId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate data segment id.");
        }
    }

    if (auto index = parseMemoryIndex(context)) {
        result->memoryIndex = *index;
    } else if (startClause(context, "memory")) {
        if (auto index = parseMemoryIndex(context)) {
            result->memoryIndex = *index;
        } else {
            msgs.error(tokens.peekToken(), "Missing or invalid memory index.");
        }

        requiredCloseParenthesis(context);
    }

    if (startClause(context, "offset")) {
        result->expression.reset(requiredExpression(context));

        requiredCloseParenthesis(context);
    } else if (tokens.getParenthesis('(')) {
        result->expression.reset(requiredExpression(context, true));

        requiredCloseParenthesis(context);
    }

    while (auto str = context.getString()) {
        result->init.append(*str);
    }

    auto flags = SegmentFlagNone;

    if (result->memoryIndex != 0) {
        flags = SegmentFlags(flags | SegmentFlagExplicitIndex);
    }

    if (!result->expression) {
        flags = SegmentFlags(flags | SegmentFlagPassive);
    }

    result->flags = flags;

    requiredCloseParenthesis(context);

    return result;
}
 
DataSegment* DataSegment::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto& msgs = context.msgs();
    auto result = context.makeTreeNode<DataSegment>();

    result->number = module->getSegmentCount();

    result->flags = SegmentFlags(data.getU32leb());
    msgs.errorWhen((result->flags > SegmentFlagMax), "Invalid segment flags.");

    if ((result->flags & SegmentFlagExplicitIndex) != 0) {
        result->memoryIndex = data.getU32leb();
    }

    if ((result->flags & SegmentFlagPassive) == 0) {
        result->expression.reset(Expression::readInit(context));
    }

    result->init = readByteArray(context);

    return result;
}

void DataSegment::check(CheckContext& context)
{
    context.msgs().errorWhen((flags > SegmentFlagMax), this, "Invalid segment flags.");

    context.checkMemoryIndex(this, memoryIndex);

    if (expression) {
        expression->check(context);
        context.checkInitExpression(expression.get(), ValueType::i32);
    }
}

void DataSegment::generate(std::ostream& os, Module* module)
{
    os << "\n  (data (;" << number << ";)";

    if ((flags & SegmentFlagExplicitIndex) != 0) {
        os << " " << memoryIndex;
    }

    if ((flags & SegmentFlagPassive) == 0) {
        os << " (";
        expression->generate(os, module);
        os << ')';
    }

    os << " \"";
    generateChars(os, init);
    os << '\"';

    os << ')';
}

void DataSegment::show(std::ostream& os, Module* module)
{
    os << "  segment " << number << ": ";
    os << "memory=" << memoryIndex;

    if ((flags & SegmentFlagPassive) == 0) {
        os << ", offset=(";
        expression->generate(os, module);
        os << ")";
    }

    os << ", init=";
    dumpChars(os, init, 0);
    os << '\n';
}

std::string DataSegment::getCName(const Module* module) const
{
    std::string prefix = module->getNamePrefix();

    return prefix + "_data_" + toString(number);

}

void DataSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::data);
    data.push();
    data.putU32leb(uint32_t(segments.size()));

    for (auto& segment : segments) {
        segment->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

DataSection* DataSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<DataSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("Data");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->segments.emplace_back(DataSegment::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error("Data section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void DataSection::check(CheckContext& context)
{
    for (auto& segment : segments) {
        segment->check(context);
    }
}

void DataSection::generate(std::ostream& os, Module* module)
{
    for (auto& segment : segments) {
        segment->generate(os, module);
    }
}

void DataSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Data section:\n";

    for (auto& segment : segments) {
        segment->show(os, module);
    }

    os << '\n';
}

DataCountSection* DataCountSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = context.makeTreeNode<DataCountSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    msgs.setSectionName("DataCount");

    result->setData(context, startPos, startPos + size);

    result->dataCount = data.getU32leb();

    if (data.getPos() != startPos + size) { 
        msgs.error("DataCount section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void DataCountSection::check(CheckContext& context)
{
    context.checkDataCount(this, dataCount);
}

void DataCountSection::generate(std::ostream& os, Module* module)
{
    // nothing to do
}

void DataCountSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "DataCount section:\n";
    os << "  data count: " << dataCount << '\n';
    os << '\n';
}

void DataCountSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::dataCount);
    data.push();
    data.putU32leb(dataCount);

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

void EventDeclaration::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(uint8_t(attribute));
    data.putU32leb(typeIndex);
}

EventDeclaration* EventDeclaration::parse(SourceContext& context)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "event")) {
        return nullptr;
    }

    auto result = context.makeTreeNode<EventDeclaration>();
    result->number = module->nextEventCount();
    module->addEvent(result);

    if (auto id = context.getId()) {
        result->id = *id;

        if (!module->addEventId(*id, result->number)) {
            msgs.error(tokens.peekToken(-1), "Duplicate event id.");
        }

    }

    result->attribute = EventType(requiredU8(context));

    if (auto index = parseTypeIndex(context)) {
        result->setIndex(*index);
    } else {
        msgs.error(tokens.peekToken(), "Missing or invalid Type index.");
    }

    requiredCloseParenthesis(context);

    return result;
}
 
EventDeclaration* EventDeclaration::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<EventDeclaration>();

    result->attribute = EventType(data.getU8());
    result->typeIndex = data.getU32leb();
    result->number = module->nextEventCount();

    return result;
}

void EventDeclaration::show(std::ostream& os, Module* module)
{
    os << "  event ";
    shsowFunctionIndex(os, number, module);

    os << ':';
    os << " type attribute=\"" << attribute << "\" ";
    os << ",  type index=\"" << typeIndex << "\" ";
    os << '\n';
}

void EventDeclaration::check(CheckContext& context)
{
    context.checkEventType(this, attribute);
    context.checkTypeIndex(this, typeIndex);
}

void EventDeclaration::generate(std::ostream& os, Module* module)
{
    os << "\n (event (;" << number << ";) " << attribute << ' ' << typeIndex;

    os << ')';
}

void EventSection::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU8(SectionType::event);
    data.push();
    data.putU32leb(uint32_t(events.size()));

    for (auto& event : events) {
        event->write(context);
    }

    auto text = data.pop();

    data.putU32leb(uint32_t(text.size()));
    data.append(text);
}

EventSection* EventSection::read(BinaryContext& context)
{
    auto* module = context.getModule();
    auto& data = context.data();
    auto result = context.makeTreeNode<EventSection>();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    auto& msgs = context.msgs();

    module->startLocalEvents();

    msgs.setSectionName("Event");

    result->setData(context, startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        msgs.setEntryNumber(i);
        result->events.emplace_back(EventDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        msgs.error(" event section not fully read");
        data.setPos(startPos + size);
    }

    msgs.resetInfo();
    return result;
}

void EventSection::check(CheckContext& context)
{
    for (auto& event : events) {
        event->check(context);
    }
}

void EventSection::generate(std::ostream& os, Module* module)
{
    for (auto& event : events) {
        event->generate(os, module);
    }
}

void EventSection::show(std::ostream& os, Module* module, unsigned flags)
{
    os << "Event section:\n";

    for (auto& event : events) {
        event->show(os, module);
    }

    os << '\n';
}

};
