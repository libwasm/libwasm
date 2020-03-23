// BackBone.cpp

#include "BackBone.h"

#include "Instruction.h"
#include "common.h"
#include "parser.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

static ValueType readValueType(BinaryContext& context)
{
    ValueType result(context.data().getI32leb());

    context.msgs().errorWhen(!result.isValid(), "Invalid ValueType ", int32_t(result));

    return result;
}

static void writeValueType(BinaryContext& context, ValueType type)
{
    context.data().putI32leb(int32_t(type));
}

static Limits readLimits(BinaryContext& context)
{
    auto& data = context.data();
    Limits result;

    result.kind = LimitKind(data.getU8());
    result.min = data.getU32leb();

    if (result.kind == LimitKind::hasMax) {
        result.max = data.getU32leb();

        context.msgs().errorWhen(result.max < result.min, "Invalid limits: max (", result.max,
                ") is less than min (", result.min, ')');
    }

    return result;
}

static void writeLimits(BinaryContext& context, const Limits& limits)
{
    auto& data = context.data();

    data.putU8(uint8_t(limits.kind));
    data.putU32leb(limits.min);

    if (limits.kind == LimitKind::hasMax) {
        data.putU32leb(limits.max);
    }
}

static ExternalKind readExternalKind(BinaryContext& context)
{
    auto& data = context.data();
    auto result = ExternalKind(data.getU8());

    context.msgs().errorWhen(!result.isValid(), "Invalid ExternalKind ", uint8_t(result));

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

static void shsowFunctionIndex(std::ostream& os, uint32_t index, Context& context)
{
    os << index;

    auto id = context.getFunction(index)->getId();

    if (!id.empty()) {
        os << " \"" << id << "\"";
    }
}

void Limits::generate(std::ostream& os)
{
    os << ' ' << min;

    if (kind == LimitKind::hasMax) {
        os << ' ' << max;
    }
}

void Limits::show(std::ostream& os)
{
    os << " min=" << min;

    if (kind == LimitKind::hasMax) {
        os << ", max=" << max;
    }
}

void Section::dump(std::ostream& os, BinaryContext& context)
{
    os << '\n' << type << " section:";
    context.data().dump(os, startOffset, endOffset);
}

CustomSection* CustomSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto size = data.getU32leb();
    auto startPos = data.getPos();
    CustomSection* result;
    std::string name = readByteArray(context);

    context.msgs().setSectionName("Custom");

    if (name.find("reloc.") == 0) {
        result = RelocationSection::read(context);
    } else if (name == "linking") {
        result = LinkingSection::read(context, startPos + size);
    } else {
        result = new CustomSection;
        context.msgs().warning("Custom section '", name, "' ignored");
    }

    result->setOffsets(startPos, startPos + size);
    result->name = name;

    //TBI debug sections
    data.setPos(startPos + size);

    if (data.getPos() != startPos + size) {
        context.msgs().error("Custom section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void CustomSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    // nothing to do
}

void CustomSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Custom section " << name << ":\n";
}

RelocationEntry* RelocationEntry::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new RelocationEntry;

    result->type = RelocationType(data.getU8());
    result->offset = data.getU32leb();
    result->index = data.getU32leb();

    if (hasAddend(result->type)) {
        result->addend = data.getI32leb();
    }

    return result;
}

void RelocationEntry::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void RelocationEntry::show(std::ostream& os, Context& context)
{
    auto flags = os.flags();

    os << "  " << type << ", offset=0x" << std::hex << offset << std::dec << ", index=" << index;

    auto* symbol = context.getSymbol(index);
    auto indexName { symbol->getName() };

    if (indexName.empty()) {
        switch (type) {
            case RelocationType::functionIndexLeb:
            {
                if (indexName.empty()) {
                    indexName = context.getFunction(symbol->getIndex())->getId();
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
    auto result = new RelocationSection;

    result->targetSectionIndex = data.getU32leb();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->relocations.emplace_back(RelocationEntry::read(context));
    }

    return result;
}

void RelocationSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    // nothing to do
}

void RelocationSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Relocation section " << name << ":";
    os << " target section=" << targetSectionIndex << '\n';

    for (auto& relocation : relocations) {
        relocation->show(os, context);
    }

    os << '\n';
}

LinkingSegmentInfo* LinkingSegmentInfo::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new LinkingSegmentInfo;

    result->name = readByteArray(context);
    result->align = data.getU32leb();
    result->flags = data.getU32leb();

    return result;
}

void LinkingSegmentInfo::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingSegmentInfo::show(std::ostream& os, Context& context)
{
    os << "    name=\"" << name << "\", align=" << align << ", flags=" << flags << '\n';
}

LinkingSegmentSubsection* LinkingSegmentSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new LinkingSegmentSubsection;

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->infos.emplace_back(LinkingSegmentInfo::read(context));
    }

    return result;
}

void LinkingSegmentSubsection::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingSegmentSubsection::show(std::ostream& os, Context& context)
{
    os << "  segment info\n";

    for (auto& info : infos) {
        info->show(os, context);
    }
}

LinkingInitFunc* LinkingInitFunc::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new LinkingInitFunc;

    result->priority = data.getU32leb();
    result->functionIndex = data.getU32leb();

    return result;
}

void LinkingInitFunc::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingInitFunc::show(std::ostream& os, Context& context)
{
    os << "    function=" << functionIndex << ", priority=" << priority << '\n';
}

LinkingInitFuncSubsection* LinkingInitFuncSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new LinkingInitFuncSubsection;

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->inits.emplace_back(LinkingInitFunc::read(context));
    }

    return result;
}

void LinkingInitFuncSubsection::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingInitFuncSubsection::show(std::ostream& os, Context& context)
{
    os << "  init functions\n";

    for (auto& init : inits) {
        init->show(os, context);
    }
}

ComdatSym* ComdatSym::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new ComdatSym;

    result->kind = ComdatSymKind(data.getU8());
    result->index = data.getU32leb();

    return result;
}

void ComdatSym::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void ComdatSym::show(std::ostream& os, Context& context)
{
    os << "      " << kind << ", index=" << index << '\n';
}

LinkingComdat* LinkingComdat::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new LinkingComdat;

    result->name = readByteArray(context);
    result->flags = data.getU8();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->syms.emplace_back(ComdatSym::read(context));
    }

    return result;
}

void LinkingComdat::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingComdat::show(std::ostream& os, Context& context)
{
    os << "    name =\"" << name << "\", flags=" << flags << "\n";

    for (auto& sym : syms) {
        sym->show(os, context);
    }
}

LinkingComdatSubsection* LinkingComdatSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new LinkingComdatSubsection;

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->comdats.emplace_back(LinkingComdat::read(context));
    }

    return result;
}

void LinkingComdatSubsection::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingComdatSubsection::show(std::ostream& os, Context& context)
{
    os << "  comdats\n";

    for (auto& comdat : comdats) {
        comdat->show(os, context);
    }
}

SymbolTableFGETInfo* SymbolTableFGETInfo::read(BinaryContext& context,
        SymbolKind kind, SymbolFlags flags)
{
    auto& data = context.data();
    auto result = new SymbolTableFGETInfo;

    result->index = data.getU32leb();

    if ((uint32_t(flags) & uint32_t(SymbolFlags::undefined)) == 0 ||
            (uint32_t(flags) & uint32_t(SymbolFlags::explicitName)) != 0) {
        result->name = readByteArray(context);

        switch(kind) {
            case SymbolKind::function:
                context.getFunction(result->index)->setId(result->name);
                break;

            default:
                break;
        }
    }

    return result;
}

void SymbolTableFGETInfo::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void SymbolTableFGETInfo::show(std::ostream& os, Context& context)
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
    auto result = new SymbolTableDataInfo;

    result->name = readByteArray(context);

    if ((uint32_t(flags) & uint32_t(SymbolFlags::undefined)) == 0) {
        result->dataIndex = data.getU32leb();
        result->offset = data.getU32leb();
        result->size = data.getU32leb();
    }

    return result;
}

void SymbolTableDataInfo::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void SymbolTableDataInfo::show(std::ostream& os, Context& context)
{
    os << "    " << kind << ", name=\"" << name << '\"';

    if ((uint32_t(flags) & uint32_t(SymbolFlags::undefined)) == 0) {
        os << ", index=" << dataIndex << ", offset-" << offset << ", size=" << size;
    }

    showFlags(os);
    os << '\n';
}

SymbolTableSectionInfo* SymbolTableSectionInfo::read(BinaryContext& context, SymbolFlags flags)
{
    auto& data = context.data();
    auto result = new SymbolTableSectionInfo;

    result->tableIndex = data.getU32leb();

    return result;
}

void SymbolTableSectionInfo::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void SymbolTableSectionInfo::show(std::ostream& os, Context& context)
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

    auto result = new SymbolTableInfo;

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

    context.addSymbol(result);

    return result;
}

void SymbolTableInfo::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void SymbolTableInfo::show(std::ostream& os, Context& context)
{
}

void SymbolTableInfo::showFlags(std::ostream& os)
{
    if (flags != SymbolFlags::none) {
        if ((uint32_t(flags) & uint32_t(SymbolFlags::weak)) != 0) {
            os << ", weak";
        }

        if ((uint32_t(flags) & uint32_t(SymbolFlags::local)) != 0) {
            os << ", local";
        }

        if ((uint32_t(flags) & uint32_t(SymbolFlags::hidden)) != 0) {
            os << ", hidden";
        }

        if ((uint32_t(flags) & uint32_t(SymbolFlags::undefined)) != 0) {
            os << ", undefined";
        }

        if ((uint32_t(flags) & uint32_t(SymbolFlags::exported)) != 0) {
            os << ", exported";
        }

        if ((uint32_t(flags) & uint32_t(SymbolFlags::explicitName)) != 0) {
            os << ", explicitName";
        }

        if ((uint32_t(flags) & uint32_t(SymbolFlags::noStrip)) != 0) {
            os << ", noStrip";
        }
    }
}

LinkingSymbolTableSubSectionn* LinkingSymbolTableSubSectionn::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new LinkingSymbolTableSubSectionn;

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->infos.emplace_back(SymbolTableInfo::read(context));
    }

    return result;
}

void LinkingSymbolTableSubSectionn::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingSymbolTableSubSectionn::show(std::ostream& os, Context& context)
{
    os << "  Symbol table\n";

    for (auto& info : infos) {
        info->show(os, context);
    }
}

LinkingSubsection* LinkingSubsection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto type = LinkingType(data.getU8());
    auto length = data.getU32leb();
    auto startPos = data.getPos();

    auto result = new LinkingSubsection;

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

void LinkingSubsection::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void LinkingSubsection::show(std::ostream& os, Context& context)
{
}

LinkingSection* LinkingSection::read(BinaryContext& context, size_t endPos)
{
    auto& data = context.data();
    auto result = new LinkingSection;

    result->version = data.getU32leb();
    context.msgs().errorWhen(result->version != wasmLinkingVersion,
            "Imvalid linking section version ", result->version);

    while (data.getPos() < endPos) {
        result->subSections.emplace_back(LinkingSubsection::read(context));
    }

    return result;
}

void LinkingSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    // nothing to do
}

void LinkingSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Linking section:\n";

    for (auto& subSection : subSections) {
        subSection->show(os, context);
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
    auto& tokens = context.tokens();
    auto result = new Signature;
    bool found = false;

    while (startClause(context, "param")) {
        found = true;
        if (auto id = tokens.getId()) {
            if (auto value = parseValueType(context)) {
                auto local = new Local(*id, *value);

                local->setNumber(context.nextLocalCount());

                result->params.emplace_back(local);
                if (!context.addLocalId(*id, local->getNumber())) {
                    context.msgs().error(tokens.peekToken(-1), "Duplicate local id.");
                }
            }
        } else {
            for (;;) {
                if (auto valueType = parseValueType(context)) {
                    auto local = new Local(*valueType);

                    local->setNumber(context.nextLocalCount());

                    result->params.emplace_back(local);
                } else {
                    break;
                }
            }
        }

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
        }
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

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
        }
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
    auto result = new Signature;

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->params.push_back(new Local(readValueType(context)));
    }

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->results.push_back(readValueType(context));
    }

    return result;
}

void Signature::generate(std::ostream& os, Context& context)
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

void Signature::show(std::ostream& os, Context& context)
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
    writeValueType(context, ValueType::func);
    signature->write(context);
}

TypeDeclaration* TypeDeclaration::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "type")) {
        return nullptr;
    }

    auto result = new TypeDeclaration;

    result->number = context.getTypeCount();

    if (auto id = tokens.getId()) {
        result->id = *id;

        if (!context.addTypeId(*id, result->number)) {
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
        result->signature.reset(new Signature);
    }

    // terminate func
    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    // terminate type
    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
TypeDeclaration* TypeDeclaration::read(BinaryContext& context)
{
    auto result = new TypeDeclaration;

    if (auto f = readValueType(context); f != ValueType::func) {
        context.msgs().error("Invalid func opcode ", std::hex, f, std::dec);
        return result;
    }

    result->signature.reset(Signature::read(context));
    result->number = context.getTypeCount();

    return result;
}

void TypeDeclaration::generate(std::ostream& os, Context& context)
{
    os << "\n  (type (;" << number << ";) (func";

    signature->generate(os, context);

    os << "))";
}

void TypeDeclaration::show(std::ostream& os, Context& context)
{
    os << "  Type " << number << ": ";

    signature->show(os, context);
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

void TypeSection::read(BinaryContext& context, TypeSection* result)
{
    auto& data = context.data();
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Type");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->types.emplace_back(TypeDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) {
        context.msgs().error("Type section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
}

void TypeSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& type : types) {
        type->generate(os, context);
    }
}

void TypeSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Type section:\n";

    for (auto& type : types) {
        type->show(os, context);
    }

    os << '\n';
}

void TypeUse::checkSignature(SourceContext& context)
{
    auto* typeSection = context.getTypeSection();

    if (signatureIndex == invalidIndex) {
        if (!signature) {
            signature.reset(new Signature);
        }

        if (typeSection == 0) {
            typeSection = new TypeSection;

            context.setTypeSectionIndex(context.getSections().size());
            context.getSections().emplace_back(typeSection);
        }

        auto& types = typeSection->getTypes();

        for (uint32_t i = 0, c = uint32_t(types.size()); i < c; ++i) {
            if (*signature == *(types)[i]->getSignature()) {
                signatureIndex = i;
                return;
            }
        }

        auto* typeDeclaration = new TypeDeclaration(new Signature(*signature));

        typeDeclaration->setNumber(context.getTypeCount());
        typeSection->getTypes().emplace_back(typeDeclaration);

        signatureIndex = typeDeclaration->getNumber();
    } else {
        auto* typeDeclaration = context.getType(signatureIndex);

        if (signature) {
            if (*signature != *typeDeclaration->getSignature()) {
                context.msgs().error(context.tokens().peekToken(-1), "Signature of function differs from indexed type.");
            }
        } else {
            signature.reset(new Signature(*typeDeclaration->getSignature()));

            for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
                context.nextLocalCount();
            }
        }
    }
}

void TypeUse::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU32leb(signatureIndex);
}

void TypeUse::parse(SourceContext& context, TypeUse* result)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (startClause(context, "type")) {
        if (auto id = tokens.getId()) {
            result->signatureIndex = context.getTypeIndex(*id);
            msgs.errorWhen(result->signatureIndex == invalidIndex, tokens.peekToken(-1),
                    "Type with id '", *id, "' does not exist.");
        } else if (auto index = tokens.getU32()) {
            result->signatureIndex = *index;
            msgs.errorWhen(*index >= context.getTypeCount(), tokens.peekToken(-1),
                    "Type index ", *index, " out of bounds.");
        }

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
            return;
        }
    }

    if (auto* sig = Signature::parse(context); sig != nullptr) {
        result->signature.reset(sig);
    }

    result->checkSignature(context);
}
 
void TypeUse::read(BinaryContext& context, TypeUse* result)
{
    auto& data = context.data();

    result->signatureIndex = data.getU32leb();

    auto* typeDeclaration = context.getType(result->signatureIndex);

    result->signature.reset(new Signature(*typeDeclaration->getSignature()));
}

void TypeUse::generate(std::ostream& os, Context& context)
{
    os << " (type " << signatureIndex << ')';
}

void TypeUse::show(std::ostream& os, Context& context)
{
    os << " signature index=\"" << signatureIndex << "\" ";
    signature->show(os, context);
}

void FunctionImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalKind::function));
    TypeUse::write(context);
}

FunctionImport* FunctionImport::parse(SourceContext& context, const std::string_view name)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "func")) {
        return nullptr;
    }

    auto result = new FunctionImport;

    context.addFunction(result);
    context.startFunction();
    result->number = context.nextFunctionCount();

    if (auto id = tokens.getId()) {
        result->id = *id;
    } else {
        result->id = name;
    }

    if (!context.addFunctionId(result->id, result->number)) {
        context.msgs().error(tokens.peekToken(-1), "Duplicate function id '", result->id, "'.");
    }

    TypeUse::parse(context, result);

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
FunctionImport* FunctionImport::read(BinaryContext& context, const std::string_view name)
{
    auto result = new FunctionImport;

    context.addFunction(result);
    result->number = context.nextFunctionCount();

    TypeUse::read(context, result);
    if (result->id.empty()) {
        result->id = name;
    }

    context.addFunctionId(name, result->number);

    return result;
}

void FunctionImport::generate(std::ostream& os, Context& context)
{
    os << "\n  (import";
    generateNames(os);
    os << " (func (;" << number << ";)";
    static_cast<TypeUse*>(this)->generate(os, context);
    os << "))";
}

void FunctionImport::show(std::ostream& os, Context& context)
{
    os << "  func " << number << ':';
    generateNames(os);
    os << ", ";
    static_cast<TypeUse*>(this)->show(os, context);
    os << '\n';
}

void MemoryImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalKind::memory));
    writeLimits(context, limits);
}

MemoryImport* MemoryImport::parse(SourceContext& context, const std::string_view name)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "memory")) {
        return nullptr;
    }

    auto result = new MemoryImport;

    result->number = context.nextMemoryCount();

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addMemoryId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate memory id.");
        }
    }

    if (auto limits = requiredLimits(context)) {
        result->limits = *limits;
    } else {
        tokens.recover();
        return result;
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
MemoryImport* MemoryImport::read(BinaryContext& context, const std::string_view name)
{
    auto result = new MemoryImport;

    result->limits = readLimits(context);
    result->number = context.nextMemoryCount();

    return result;
}

void MemoryImport::generate(std::ostream& os, Context& context)
{
    os << "\n  (import";
    generateNames(os);
    os << " (memory (;" << number << ";)";
    limits.generate(os);
    os << "))";
}

void MemoryImport::show(std::ostream& os, Context& context)
{
    os << "  memory " << number << ':';
    generateNames(os);
    os << ", ";
    limits.show(os);
    os << '\n';
}

void TableImport::write(BinaryContext& context) const
{
    auto& data = context.data();

    writeByteArray(context, moduleName);
    writeByteArray(context, name);

    data.putU8(uint8_t(ExternalKind::table));
    writeValueType(context, type);
    writeLimits(context, limits);
}

TableImport* TableImport::parse(SourceContext& context, const std::string_view name)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "table")) {
        return nullptr;
    }

    auto result = new TableImport;

    context.addTable(result);
    result->number = context.nextTableCount();

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addTableId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate table id.");
        }
    }

    if (auto limits = requiredLimits(context)) {
        result->limits = *limits;
    } else {
        tokens.recover();
        return result;
    }

    if (auto valueType = parseValueType(context)) {
        result->type = *valueType;
        context.msgs().errorWhen(valueType != ValueType::funcref, tokens.peekToken(-1), "ValueType must be 'funcref'");
    } else {
        context.msgs().expected(tokens.peekToken(), "funcref");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
TableImport* TableImport::read(BinaryContext& context, const std::string_view name)
{
    auto result = new TableImport;

    context.addTable(result);
    result->type = readValueType(context);
    result->limits = readLimits(context);
    result->number = context.nextTableCount();

    return result;
}

void TableImport::generate(std::ostream& os, Context& context)
{
    os << "\n  (import";
    generateNames(os);
    os << " (table (;" << number << ";)";
    limits.generate(os);
    os << ' ' << type << "))";
}

void TableImport::show(std::ostream& os, Context& context)
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

    data.putU8(uint8_t(ExternalKind::global));
    writeValueType(context, type);
    data.putU8(uint8_t(mut));
}

GlobalImport* GlobalImport::parse(SourceContext& context, const std::string_view name)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "global")) {
        return nullptr;
    }

    auto result = new GlobalImport;

    result->number = context.nextGlobalCount();
    context.addGlobal(result);

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addGlobalId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate global id.");
        }
    }

    if (startClause(context, "mut")) {
        result->mut = Mut::var;
        if (auto valueType = parseValueType(context)) {
            result->type = *valueType;
            if (!requiredParenthesis(context, ')')) {
                tokens.recover();
                return result;
            }
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

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
GlobalImport* GlobalImport::read(BinaryContext& context, const std::string_view name)
{
    auto& data = context.data();
    auto result = new GlobalImport;

    result->type = readValueType(context);
    result->mut = Mut(data.getU8());
    result->number = context.nextGlobalCount();

    return result;
}

void GlobalImport::generate(std::ostream& os, Context& context)
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

void GlobalImport::show(std::ostream& os, Context& context)
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
    auto& tokens = context.tokens();

    if (!startClause(context, "func")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = tokens.getId();

    if (!startClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    auto& msgs = context.msgs();
    auto result = new FunctionImport;

    context.addFunction(result);
    context.startFunction();
    result->number = context.nextFunctionCount();

    if (auto value = tokens.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = tokens.getString()) {
        result->setName(*value);
        result->setId(id ? *id : *value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    if (!context.addFunctionId(result->getId(), result->number)) {
        context.msgs().error(tokens.peekToken(-1), "Duplicate function id '", result->getId(), "'.");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    TypeUse::parse(context, result);

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}

ImportDeclaration* ImportDeclaration::parseTableImport(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "table")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = tokens.getId();

    if (!startClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    auto& msgs = context.msgs();
    auto result = new TableImport;

    context.addTable(result);
    result->number = context.nextTableCount();

    if (auto value = tokens.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = tokens.getString()) {
        result->setName(*value);
        result->setId(id ? *id : *value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    if (!context.addTableId(result->getId(), result->number)) {
        context.msgs().error(tokens.peekToken(-1), "Duplicate table id '", result->getId(), "'.");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    if (auto limits = requiredLimits(context)) {
        result->setLimits(*limits);
    } else {
        tokens.recover();
        return result;
    }

    if (auto valueType = parseValueType(context)) {
        result->setType(*valueType);
        context.msgs().errorWhen(valueType != ValueType::funcref, tokens.peekToken(-1), "ValueType must be 'funcref'");
    } else {
        context.msgs().expected(tokens.peekToken(), "funcref");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}

ImportDeclaration* ImportDeclaration::parseMemoryImport(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "memory")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = tokens.getId();

    if (!startClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    auto& msgs = context.msgs();
    auto result = new MemoryImport;

    context.addMemory(result);
    result->number = context.nextMemoryCount();

    if (auto value = tokens.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = tokens.getString()) {
        result->setName(*value);
        result->setId(id ? *id : *value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    if (!context.addMemoryId(result->getId(), result->number)) {
        context.msgs().error(tokens.peekToken(-1), "Duplicate memory id '", result->getId(), "'.");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    if (auto limits = requiredLimits(context)) {
        result->setLimits(*limits);
    } else {
        tokens.recover();
        return result;
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}

ImportDeclaration* ImportDeclaration::parseGlobalImport(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "global")) {
        return nullptr;
    }

    auto startPos = tokens.getPos();

    auto id = tokens.getId();

    if (!startClause(context, "import")) {
        tokens.setPos(startPos);
        return nullptr;
    }

    auto& msgs = context.msgs();
    auto result = new GlobalImport;

    context.addGlobal(result);
    result->number = context.nextGlobalCount();

    if (auto value = tokens.getString()) {
        result->setModuleName(*value);
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = tokens.getString()) {
        result->setName(*value);
        result->setId(id ? *id : *value);
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    if (!context.addGlobalId(result->getId(), result->number)) {
        context.msgs().error(tokens.peekToken(-1), "Duplicate global id '", result->getId(), "'.");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    if (startClause(context, "mut")) {
        result->setMut(Mut::var);
        if (auto valueType = parseValueType(context)) {
            result->setType(*valueType);
            if (!requiredParenthesis(context, ')')) {
                tokens.recover();
                return result;
            }
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

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

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

    if (auto value = tokens.getString()) {
        moduleName = *value;
    } else {
        msgs.expected(tokens.peekToken(), "module name");
    }

    if (auto value = tokens.getString()) {
        name = *value;
    } else {
        msgs.expected(tokens.peekToken(), "name");
    }

    ImportDeclaration* result = nullptr;

    if (auto entry = FunctionImport::parse(context, name); entry) {
        result = entry;
    } else if (auto entry = TableImport::parse(context, name); entry) {
        result = entry;
    } else if (auto entry = MemoryImport::parse(context, name); entry) {
        result = entry;
    } else if (auto entry = GlobalImport::parse(context, name); entry) {
        result = entry;
    } else {
        msgs.expected(tokens.peekToken(), "one of '(memory', '(global', '(func' or '(table'");
        tokens.recover();
        return result;
    }

    result->setModuleName(moduleName);
    result->setName(name);

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

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
    auto result = new ImportSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Import");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        auto moduleName = readByteArray(context);
        auto name = readByteArray(context);

        ImportDeclaration* import = nullptr;
        auto kind = readExternalKind(context);

        switch (uint8_t(kind)) {
            case ExternalKind::function: import = FunctionImport::read(context, name); break;
            case ExternalKind::table:    import = TableImport::read(context, name); break;
            case ExternalKind::memory:   import = MemoryImport::read(context, name); break;
            case ExternalKind::global:   import = GlobalImport::read(context, name); break;
            default:
                context.msgs().error("Invalid import declaration ", uint8_t(kind));
                break;
        }

        if (import == nullptr) {
            break;
        }

        import->setModuleName(moduleName);
        import->setName(name);

        context.addImport(import);
        result->imports.emplace_back(import);
    }

    if (data.getPos() != startPos + size) {
        context.msgs().error("Import section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void ImportSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& import : imports) {
        import->generate(os, context);
    }
}

void ImportSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Import section:\n";

    for (auto& import : imports) {
        import->show(os, context);
    }

    os << '\n';
}

void FunctionDeclaration::write(BinaryContext& context) const
{
    TypeUse::write(context);
}

FunctionDeclaration* FunctionDeclaration::parse(SourceContext& context, CodeEntry*& codeEntry)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "func")) {
        return nullptr;
    }

    auto result = new FunctionDeclaration;

    context.addFunction(result);
    context.startFunction();

    result->number = context.nextFunctionCount();

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addFunctionId(result->id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate function id.");
        }
    }

    while (startClause(context, "export")) {
        auto* _export = new ExportDeclaration;

        _export->setKind(ExternalKind::function);
        _export->setNumber(context.nextExportCount());
        _export->setName(requiredString(context));
        _export->setIndex(result->number);

        context.addExport(_export);

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
            return result;
        }
    }

    TypeUse::parse(context, result);
    codeEntry = CodeEntry::parse(context);

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
FunctionDeclaration* FunctionDeclaration::read(BinaryContext& context)
{
    auto result = new FunctionDeclaration;

    context.addFunction(result);

    TypeUse::read(context, result);
    result->number = context.nextFunctionCount();

    return result;
}

void FunctionDeclaration::generate(std::ostream& os, Context& context)
{
    // nothing to do
}

void FunctionDeclaration::show(std::ostream& os, Context& context)
{
    os << "  func " << number;

    if (!id.empty()) {
        os << " \"" << id << "\"";
    }

    os << ": ";
    static_cast<TypeUse*>(this)->show(os, context);
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
    auto& data = context.data();
    auto result = new FunctionSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Function");

    result->setOffsets(startPos, startPos + size);

    context.startLocalFunctions();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->functions.emplace_back(FunctionDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Function section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void FunctionSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    // nothing to do
}

void FunctionSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Function section:\n";

    for (auto& function : functions) {
        function->show(os, context);
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
    auto& tokens = context.tokens();

    if (!startClause(context, "table")) {
        return nullptr;
    }

    auto result = new TableDeclaration;

    context.addTable(result);
    result->number = context.nextTableCount();

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addTableId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate table id.");
        }
    }

    if (auto valueType = parseValueType(context)) {
        result->type = *valueType;
        context.msgs().errorWhen(valueType != ValueType::funcref, tokens.peekToken(-1), "ValueType must be 'funcref'");
        if (startClause(context, "elem")) {
            auto element = new ElementDeclaration;
            uint32_t functionIndexCount = 0;

            element->setNumber(context.nextElementCount());
            element->setTableIndex(result->getNumber());
            while (auto index = parseFunctionIndex(context)) {
                element->addFunctionIndex(*index);
                ++functionIndexCount;
            }

            auto* expression = new Expression;
            auto* instruction = new InstructionI32;

            instruction->setOpcode(Opcode::i32__const);
            expression->addInstruction(instruction);

            element->setExpression(expression);
            if (!requiredParenthesis(context, ')')) {
                tokens.recover();
                return result;
            }

            context.addElement(element);

            result->limits.min = functionIndexCount;
            result->limits.max = functionIndexCount;
            result->limits.kind = LimitKind::hasMax;
        } else {
            context.msgs().expected(tokens.peekToken(), "(elem");
            tokens.recover();
            return result;
        }
    } else if (auto limits = requiredLimits(context)) {
        result->limits = *limits;

        if (auto valueType = parseValueType(context)) {
            result->type = *valueType;
            context.msgs().errorWhen(valueType != ValueType::funcref, tokens.peekToken(-1), "ValueType must be 'funcref'");
        } else {
            context.msgs().expected(tokens.peekToken(), "funcref");
        }
    } else {
        tokens.recover();
        return result;
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
TableDeclaration* TableDeclaration::read(BinaryContext& context)
{
    auto result = new TableDeclaration;

    context.addTable(result);
    result->type = readValueType(context);
    context.msgs().errorWhen(result->type != ValueType::funcref, "ValueType must be 'funcref'");

    result->limits = readLimits(context);
    result->number = context.nextTableCount();

    return result;
}

void TableDeclaration::show(std::ostream& os, Context& context)
{
    os << "  table ";
    shsowFunctionIndex(os, number, context);

    os << ": type=" << type << ',';
    limits.show(os);
    os << '\n';
}

void TableDeclaration::generate(std::ostream& os, Context& context)
{
    os << "\n  (table (;" << number << ";)";
    limits.generate(os);
    os << ' ' << type;

    os << ')';
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
    auto& data = context.data();
    auto result = new TableSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Table");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->tables.emplace_back(TableDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Table section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void TableSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& table : tables) {
        table->generate(os, context);
    }
}

void TableSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Table section:\n";

    for (auto& table : tables) {
        table->show(os, context);
    }

    os << '\n';
}

void MemoryDeclaration::write(BinaryContext& context) const
{
    writeLimits(context, limits);
}

MemoryDeclaration* MemoryDeclaration::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "memory")) {
        return nullptr;
    }

    auto result = new MemoryDeclaration;
    result->number = context.nextMemoryCount();
    context.addMemory(result);

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addMemoryId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate memory id.");
        }

    }

    if (auto limits = requiredLimits(context)) {
        result->limits = *limits;
    } else {
        tokens.recover();
        return result;
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
MemoryDeclaration* MemoryDeclaration::read(BinaryContext& context)
{
    auto result = new MemoryDeclaration;

    result->limits = readLimits(context);
    result->number = context.nextMemoryCount();

    return result;
}

void MemoryDeclaration::show(std::ostream& os, Context& context)
{
    os << "  memory ";
    shsowFunctionIndex(os, number, context);

    os << ':';
    limits.show(os);
    os << '\n';
}

void MemoryDeclaration::generate(std::ostream& os, Context& context)
{
    os << "\n  (memory (;" << number << ";)";
    limits.generate(os);

    os << ')';
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
    auto& data = context.data();
    auto result = new MemorySection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Memory");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->memories.emplace_back(MemoryDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error(" memory section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void MemorySection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& memory : memories) {
        memory->generate(os, context);
    }
}

void MemorySection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Memory section:\n";

    for (auto& memory : memories) {
        memory->show(os, context);
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
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "global")) {
        return nullptr;
    }

    auto result = new GlobalDeclaration;

    result->number = context.nextGlobalCount();
    context.addGlobal(result);

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addGlobalId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate global id.");
        }
    }

    if (startClause(context, "mut")) {
        result->mut = Mut::var;
        if (auto valueType = parseValueType(context)) {
            result->type = *valueType;
            if (!requiredParenthesis(context, ')')) {
                tokens.recover();
                return result;
            }
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

    result->expression.reset(Expression::parse(context));

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
GlobalDeclaration* GlobalDeclaration::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new GlobalDeclaration;

    result->type = readValueType(context);
    result->mut = Mut(data.getU8());
    result->number = context.nextGlobalCount();
    result->expression.reset(Expression::readInit(context));

    return result;
}

void GlobalDeclaration::show(std::ostream& os, Context& context)
{
    os << "  global " << number << ": ";
    if (mut == Mut::var) {
        os << "mut ";
    }

    expression->show(os, context);
    os << '\n';

}

void GlobalDeclaration::generate(std::ostream& os, Context& context)
{
    os << "\n  (global (;" << number << ";)";

    if (mut == Mut::var) {
        os << " (mut " << type << ')';
    } else {
        os << ' ' << type;
    }

    os << " (";
    expression->generate(os, context);
    os << ')';

    os << ')';
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
    auto& data = context.data();
    auto result = new GlobalSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Global");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->globals.emplace_back(GlobalDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Global section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void GlobalSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& global : globals) {
        global->generate(os, context);
    }
}

void GlobalSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Global section:\n";

    for (auto& global : globals) {
        global->show(os, context);
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
    auto& tokens = context.tokens();

    if (!startClause(context, "export")) {
        return nullptr;
    }

    auto result = new ExportDeclaration;

    result->number = context.nextExportCount();
    result->name = requiredString(context);

    if (!requiredParenthesis(context, '(')) {
        tokens.recover();
        return result;
    }

    if (auto kind = parseExternalKind(context)) {
        result->kind = *kind;
        switch(uint8_t(*kind)) {
            case ExternalKind::function:
                if (auto index = parseFunctionIndex(context)) {
                    result->index = *index;
                } else {
                    context.msgs().error(tokens.peekToken(), "Missing or invalid function index.");
                }

                break;

            case ExternalKind::table:
                if (auto index = parseTableIndex(context)) {
                    result->index = *index;
                } else {
                    context.msgs().error(tokens.peekToken(), "Missing or invalid Table index.");
                }

                break;

            case ExternalKind::memory:
                if (auto index = parseMemoryIndex(context)) {
                    result->index = *index;
                } else {
                    context.msgs().error(tokens.peekToken(), "Missing or invalid memory index.");
                }

                break;

            case ExternalKind::global:
                if (auto index = parseGlobalIndex(context)) {
                    result->index = *index;
                } else {
                    context.msgs().error(tokens.peekToken(), "Missing or invalid global index.");
                }

                break;

            default:
                break;
        }

    } else {
        context.msgs().error(tokens.peekToken(), "Missing or invalid export kind.");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
ExportDeclaration* ExportDeclaration::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new ExportDeclaration;

    result->name = readByteArray(context);
    result->kind = readExternalKind(context);
    result->index = data.getU32leb();
    result->number = context.nextExportCount();

    return result;
}

void ExportDeclaration::show(std::ostream& os, Context& context)
{
    os << "  export " << number << ": name=\"" << name << "\", kind=" << kind << ", index=" << index;

    os << '\n';

}

void ExportDeclaration::generate(std::ostream& os, Context& context)
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
    auto result = new ExportSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Export");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->exports.emplace_back(ExportDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Export section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void ExportSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& export_ : exports) {
        export_->generate(os, context);
    }
}

void ExportSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Export section:\n";

    for (auto& export_ : exports) {
        export_->show(os, context);
    }

    os << '\n';
}

StartSection* StartSection::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "start")) {
        return nullptr;
    }

    auto result = new StartSection;

    if (auto index = parseFunctionIndex(context)) {
        result->functionIndex = *index;
    } else {
        context.msgs().error(tokens.peekToken(), "Missing or invalid function index.");
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}

StartSection* StartSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new StartSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Start");

    result->setOffsets(startPos, startPos + size);

    result->functionIndex = data.getU32leb();

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Start section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void StartSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    os << "\n  (start " << functionIndex << ')';
}

void StartSection::show(std::ostream& os, Context& context, unsigned flags)
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
    auto result = new Expression;

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

    return result;
}
 
Expression* Expression::read(BinaryContext& context)
{
    auto result = new Expression;

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
    auto result = new Expression;

    while (data.getPos() < endPos) {
        result->instructions.emplace_back(Instruction::read(context));
    }

    return result;
}

Expression* Expression::readInit(BinaryContext& context)
{
    auto result = new Expression;

    result->instructions.emplace_back(Instruction::read(context));

    auto instruction = Instruction::read(context);
    context.msgs().errorWhen(instruction->getOpcode() != Opcode::end,
            "Init expression must be terminated with 'end' instruction");

    return result;
}

void Expression::generate(std::ostream& os, Context& context)
{
    const char* separator = "";
    InstructionContext instructionContext;
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

void Expression::show(std::ostream& os, Context& context)
{
    generate(os, context);
}

void ElementDeclaration::write(BinaryContext& context) const
{
    auto& data = context.data();

    data.putU32leb(tableIndex);
    expression->write(context);

    data.putU32leb(uint32_t(functionIndexes.size()));

    for (auto f : functionIndexes) {
        data.putU32leb(f);
    }
}

ElementDeclaration* ElementDeclaration::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "elem")) {
        return nullptr;
    }

    auto result = new ElementDeclaration;

    result->number = context.nextElementCount();

    if (auto index = parseTableIndex(context)) {
        result->tableIndex = *index;
    }

    if (startClause(context, "offset")) {
        result->expression.reset(Expression::parse(context));
    } else if (requiredParenthesis(context, '(')) {
        result->expression.reset(Expression::parse(context, true));
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    while (auto index = parseFunctionIndex(context)) {
        result->functionIndexes.push_back(*index);
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
ElementDeclaration* ElementDeclaration::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new ElementDeclaration;

    result->tableIndex = data.getU32leb();
    result->expression.reset(Expression::readInit(context));
    result->number = context.nextElementCount();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        result->functionIndexes.push_back(data.getU32leb());
    }

    return result;
}

void ElementDeclaration::generate(std::ostream& os, Context& context)
{
    os << "\n  (elem (;" << number << ";)";

    if (tableIndex != 0) {
        os << ' ' << tableIndex;
    }

    os << " (";
    expression->generate(os, context);
    os << ')';

    for (auto func : functionIndexes) {
        os << ' ' << func;
    }

    os << ')';
}

void ElementDeclaration::show(std::ostream& os, Context& context)
{
    os << "  segment " << number << ": " << "table=" << tableIndex << ", offset=(";
    expression->generate(os, context);
    os << "), funcs=[";

    const char* separator = "";

    for (auto func : functionIndexes) {
        os << separator << func;
        separator = ", ";
    }

    os << "]\n";
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
    auto result = new ElementSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Element");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->elements.emplace_back(ElementDeclaration::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Element section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void ElementSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& element : elements) {
        element->generate(os, context);
    }
}

void ElementSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Element section:\n";

    for (auto& element : elements) {
        element->show(os, context);
    }

    os << '\n';
}

Local* Local::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();
    auto result = new Local;

    result->number = context.nextLocalCount();

    if (auto id = tokens.getId()) {
        result->id = *id;
        if (!context.addLocalId(*id, result->number)) {
            context.msgs().error(tokens.peekToken(-1), "Duplicate local id.");
        }
    }

    if (auto type = parseValueType(context)) {
        result->type = *type;
    } else {
        msgs.expected(tokens.peekToken(), "Value type");
    }

    return result;
}

void Local::generate(std::ostream& os, Context& context)
{
    os << ' ' << type;
}

void Local::show(std::ostream& os, Context& context)
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
    auto& tokens = context.tokens();
    auto result = new CodeEntry;

    result->number = context.nextCodeCount();

    while (startClause(context, "local")) {
        while (!tokens.peekParenthesis(')')) {
            result->locals.emplace_back(Local::parse(context));
        }

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
        }
    }

    result->expression.reset(Expression::parse(context));

    return result;
}
 
CodeEntry* CodeEntry::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new CodeEntry;

    auto size = data.getU32leb();
    auto startPos = data.getPos();

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        auto localCount = data.getU32leb();
        auto type = readValueType(context);

        for (uint32_t j = 0; j < localCount; ++j) {
            result->locals.emplace_back(new Local(type));
        }
    }

    result->expression.reset(Expression::read(context, startPos + size));
    result->number = context.nextCodeCount();

    return result;
}

void CodeEntry::generate(std::ostream& os, Context& context)
{
    auto* function = context.getFunction(number);
    auto signatureIndex = function->getSignatureIndex();

    os << "\n  (func (;" << number << ";) (type " << signatureIndex << ") ";

    function->getSignature()->generate(os, context);

    if (!locals.empty()) {
        os << "\n    (local";

        for (auto& local : locals) {
            local->generate(os, context);
        }

        os << ')';
    }

    std::string indent = "";

    auto count = expression->getInstructions().size();
    InstructionContext instructionContext;

    for (auto& instruction : expression->getInstructions()) {
        if (--count == 0 && instruction->getOpcode() == Opcode::end) {
            break;
        }

        if (indent.size() > 1 && instruction->getOpcode() == Opcode::end) {
            indent.resize(indent.size() - 2);
        }

        os << "\n    " << indent;

        instruction->generate(os, instructionContext);

        if (instruction->getParameterEncoding() == ParameterEncoding::block) {
            indent.append("  ");
        }
    }


    os << ")";
}

void CodeEntry::show(std::ostream& os, Context& context)
{
    for (auto& local : locals) {
        local->show(os, context);
    }

    std::string indent = "";
    InstructionContext instructionContext;

    for (auto& instruction : expression->getInstructions()) {
        if (indent.size() > 1 && instruction->getOpcode() == Opcode::end) {
            indent.resize(indent.size() - 2);
        }

        os << "    " << indent;

        instruction->generate(os, instructionContext);

        if (instruction->getParameterEncoding() == ParameterEncoding::block) {
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
    auto result = new CodeSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Code");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->codes.emplace_back(CodeEntry::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Code section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void CodeSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& code : codes) {
        code->generate(os, context);
    }
}

void CodeSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Code section:\n";

    auto count = context.getLocalFunctionStart();

    for (auto& code : codes) {
        os << "  Code for function ";
        shsowFunctionIndex(os, count++, context);

        if ((flags & 1) != 0) {
            os <<  ":\n";
            code->show(os, context);
        } else {
            os << '\n';
        }
    }

    os << '\n';
}

void DataSegment::write(BinaryContext& context) const
{
    auto& data = context.data();
    data.putU32leb(memoryIndex);
    expression->write(context);
    writeByteArray(context, init);
}

DataSegment* DataSegment::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "data")) {
        return nullptr;
    }

    auto result = new DataSegment;

    result->number = context.nextSegmentCount();

    if (auto index = parseTableIndex(context)) {
        result->memoryIndex = *index;
    }

    if (startClause(context, "offset")) {
        result->expression.reset(Expression::parse(context));
    } else if (requiredParenthesis(context, '(')) {
        result->expression.reset(Expression::parse(context, true));
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    while (auto str = tokens.getString()) {
        auto [error, data] = unEscape(*str);

        if (!error.empty()) {
            context.msgs().error(tokens.peekToken(-1), "Invalid string; ", error);
        }

        result->init.append(data);
    }

    if (!requiredParenthesis(context, ')')) {
        tokens.recover();
        return result;
    }

    return result;
}
 
DataSegment* DataSegment::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new DataSegment;

    result->memoryIndex = data.getU32leb();
    result->expression.reset(Expression::readInit(context));
    result->init = readByteArray(context);
    result->number = context.nextSegmentCount();

    return result;
}

void DataSegment::generate(std::ostream& os, Context& context)
{
    os << "\n  (data (;" << number << ";) (";
    expression->generate(os, context);
    os << ") \"";
    generateChars(os, init);
    os << '\"';

    os << ')';
}

void DataSegment::show(std::ostream& os, Context& context)
{
    os << "  segment " << number << ": ";
    os << "memory=" << memoryIndex << ", offset=(";
    expression->generate(os, context);
    os << "), init=";
    dumpChars(os, init, 0);
    os << '\n';
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
    auto result = new DataSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("Data");

    result->setOffsets(startPos, startPos + size);

    for (unsigned i = 0, count = unsigned(data.getU32leb()); i < count; i++) {
        context.msgs().setEntryNumber(i);
        result->segments.emplace_back(DataSegment::read(context));
    }

    if (data.getPos() != startPos + size) { 
        context.msgs().error("Data section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void DataSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    for (auto& segment : segments) {
        segment->generate(os, context);
    }
}

void DataSection::show(std::ostream& os, Context& context, unsigned flags)
{
    os << "Data section:\n";

    for (auto& segment : segments) {
        segment->show(os, context);
    }

    os << '\n';
}

DataCountSection* DataCountSection::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new DataCountSection;
    auto size = data.getU32leb();
    auto startPos = data.getPos();

    context.msgs().setSectionName("DataCount");

    result->setOffsets(startPos, startPos + size);

    result->dataCount = data.getU32leb();

    if (data.getPos() != startPos + size) { 
        context.msgs().error("DataCount section not fully read");
        data.setPos(startPos + size);
    }

    context.msgs().resetInfo();
    return result;
}

void DataCountSection::generate(std::ostream& os, Context& context, unsigned flags)
{
    // nothing to do
}

void DataCountSection::show(std::ostream& os, Context& context, unsigned flags)
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

