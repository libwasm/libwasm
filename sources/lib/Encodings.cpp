// Encodings.cpp

#include "Encodings.h"

#include <algorithm>

namespace libwasm
{

std::optional<Opcode> Opcode::fromString(std::string_view name)
{
    auto hashValue = libwasm::hash(name) % std::size(nameIndexes);
    auto& index = nameIndexes[hashValue];

    for (const NameEntry* entry = nameEntries + index.index, *endEntry = entry + index.size;
            entry != endEntry; ++entry) {
        if (entry->name == name) {
            return entry->opcode;
        }
    }

    return {};
}

const Opcode::Info* Opcode::getInfo() const
{
    auto hashValue = hash() % std::size(opcodeIndexes);
    auto& index = opcodeIndexes[hashValue];

    for (const OpcodeEntry* entry = opcodeEntries + index.index, *endEntry = entry + index.size;
            entry != endEntry; ++entry) {
        if (entry->opcode == value) {
            return info + entry->infoIndex;
        }
    }

    return nullptr;
}

std::string_view Opcode::getName() const
{
    if (auto* info = getInfo(); info == nullptr) {
        return "<unknown>";
    } else {
        return info->name;
    }
}

bool Opcode::isValid() const
{
    return getInfo() != nullptr;
}

ImmediateType Opcode::getImmediateType() const
{
    if (auto* info = getInfo(); info == nullptr) {
        return ImmediateType::none;
    } else {
        return info->type;
    }
}

uint32_t Opcode::getAlign() const
{
    if (auto* info = getInfo(); info == nullptr) {
        return 0;
    } else {
        return info->align;
    }

    return info[value].align;
}

bool SectionType::isValid() const
{
    return value <= max;
}

std::string_view SectionType::getName() const
{
    switch (value) {
        case SectionType::custom:       return "Custom";
        case SectionType::type:         return "Type";
        case SectionType::import:       return "Import";
        case SectionType::function:     return "Function";
        case SectionType::table:        return "Table";
        case SectionType::memory:       return "Memory";
        case SectionType::global:       return "Global";
        case SectionType::event:        return "Event";
        case SectionType::export_:      return "Export";
        case SectionType::start:        return "Start";
        case SectionType::element:      return "Element";
        case SectionType::code:         return "Code";
        case SectionType::data:         return "Data";
        case SectionType::dataCount:    return "DataCount";
    }

    return std::string_view();
}

bool ValueType::isValid() const
{
    switch (value) {
        case i32:
        case i64:
        case f32:
        case f64:
        case v128:
        case anyref:
        case exnref:
        case funcref:
        case nullref:
        case void_:
            return true;

        default:
            return false;
    }
}

bool ValueType::isValidNumeric() const
{
    switch (value) {
        case i32:
        case i64:
        case f32:
        case f64:
        case v128:
            return true;

        default:
            return false;
    }
}

bool ValueType::isValidRef() const
{
    switch (value) {
        case anyref:
        case exnref:
        case funcref:
        case nullref:
            return true;

        default:
            return false;;
    }
}

std::string_view ValueType::getName() const
{
    switch (value) {
        case i32:          return "i32";
        case i64:          return "i64";
        case f32:          return "f32";
        case f64:          return "f64";
        case v128:         return "v128";
        case anyref:       return "anyref";
        case exnref:       return "exnref";
        case nullref:      return "nullref";
        case funcref:      return "funcref";
        case void_:        return "void";

        default:           return std::string_view();
    }
}

std::string_view ValueType::getCName() const
{
    switch (value) {
        case i32:          return "int32_t";
        case i64:          return "int64_t";
        case f32:          return "float";
        case f64:          return "double";
        case v128:         return "v128_t";
        case anyref:       return "void *";
        case exnref:       return "exnref";
        case nullref:      return "0";
        case funcref:      return "funcref";
        case void_:        return "void";

        default:           return std::string_view();
    }
}

std::string_view ValueType::getCNullValue() const
{
    if (value == v128) {
        return "{ 0, 0 }";
    } else {
        return "0";
    }
}

struct ValueTypeEntry
{
    std::string_view name;
    ValueType encoding;
};

ValueTypeEntry valueTypeMap[] = {
    { "anyref", ValueType::anyref },
    { "exnref", ValueType::exnref },
    { "f32", ValueType::f32 },
    { "f64", ValueType:: f64},
    { "funcref", ValueType::funcref },
    { "i32", ValueType::i32 },
    { "i64", ValueType::i64 },
    { "nullref", ValueType::nullref },
    { "v128", ValueType::v128 },
    { "void", ValueType::void_ }
};

std::optional<ValueType> ValueType::getEncoding(std::string_view n)
{
    if (auto it = std::lower_bound(std::begin(valueTypeMap), std::end(valueTypeMap),
                ValueTypeEntry{n, ValueType::void_},
                [](const ValueTypeEntry& x, const ValueTypeEntry& y) {
                return x.name < y.name; });
            it != std::end(valueTypeMap) && it->name == n) {
        return it->encoding;
    }

    return {};
}

std::string_view EventType::getName() const
{
    switch (value) {
        case exception: return "exception";
    }

    return std::string_view();
}

std::string_view ExternalType::getName() const
{
    switch (value) {
        case function:  return "func";
        case table:     return "table";
        case memory:    return "memory";
        case global:    return "global";
        case event:      return "event";
    }

    return std::string_view();
}

bool ExternalType::isValid() const
{
    switch (value) {
        case function:
        case table:
        case memory:
        case global:
        case event:
            return true;
    }

    return false;
}

std::optional<ExternalType> ExternalType::getEncoding(std::string_view name)
{
    if (name == "func") {
        return function;
    }

    if (name == "table") {
        return table;
    }

    if (name == "memory") {
        return memory;
    }

    if (name == "global") {
        return global;
    }

    if (name == "event") {
        return event;
    }

    return {};
}

std::ostream& operator<<(std::ostream& os, RelocationType type)
{
    const char* name = "<unknown>";

    switch (type) {
        case RelocationType::functionIndexLeb:     name = "R_WASM_FUNCTION_INDEX_LEB"; break;
        case RelocationType::tableIndexSleb:       name = "R_WASM_TABLE_INDEX_SLEB"; break;
        case RelocationType::tableIndexI32:        name = "R_WASM_TABLE_INDEX_I32"; break;
        case RelocationType::memoryAddrLeb:        name = "R_WASM_MEMORY_ADDR_LEB"; break;
        case RelocationType::memoryAddrSleb:       name = "R_WASM_MEMORY_ADDR_SLEB"; break;
        case RelocationType::memoryAddrI32:        name = "R_WASM_MEMORY_ADDR_I32"; break;
        case RelocationType::typeIndexLeb:         name = "R_WASM_TYPE_INDEX_LEB"; break;
        case RelocationType::globaLIndexLeb:       name = "R_WASM_GLOBAL_INDEX_LEB"; break;
        case RelocationType::functionOffsetI32:    name = "R_WASM_FUNCTION_OFFSET_I32"; break;
        case RelocationType::sectionOffsetI32:     name = "R_WASM_SECTION_OFFSET_I32"; break;
        case RelocationType::eventIndexLeb:        name = "R_WASM_EVENT_INDEX_LEB"; break;
        case RelocationType::memoryAddrRelSleb:    name = "R_WASM_MEMORY_ADDR_REL_SLEB"; break;
        case RelocationType::tableIndexRelSleb:    name = "R_WASM_TABLE_INDEX_REL_SLEB"; break;
    }

    os << name;
    return os;
}

std::ostream& operator<<(std::ostream& os, LinkingType type)
{
    const char* name = "<unknown>";

    switch (type) {
        case LinkingType::segmentInfo:  name ="WASM_SEGMENT_INFO"; break;
        case LinkingType::initFuncs:    name ="WASM_INIT_FUNCS"; break;
        case LinkingType::comDatInfo:   name ="WASM_COMDAT_INFO"; break;
        case LinkingType::symbolTable:  name ="WASM_SYMBOL_TABLE"; break;
    }

    os << name;
    return os;
}

std::ostream& operator<<(std::ostream& os, ComdatSymKind type)
{
    const char* name = "<unknown>";

    switch (type) {
        case ComdatSymKind::data:       name ="WASM_COMDAT_DATA"; break;
        case ComdatSymKind::function:   name ="WASM_COMDAT_FUNCTION"; break;
        case ComdatSymKind::global:     name ="WASM_COMDAT_GLOBAL"; break;
        case ComdatSymKind::event:      name ="WASM_COMDAT_EVENT"; break;
        case ComdatSymKind::table:      name ="WASM_COMDAT_TABLE"; break;
    }

    os << name;
    return os;
}

std::ostream& operator<<(std::ostream& os, SymbolKind type)
{
    const char* name = "<unknown>";

    switch (type) {
        case SymbolKind::function:  name ="SYMTAB_FUNCTION"; break;
        case SymbolKind::data:      name ="SYMTAB_DATA"; break;
        case SymbolKind::global:    name ="SYMTAB_GLOBAL"; break;
        case SymbolKind::section:   name ="SYMTAB_SECTION"; break;
        case SymbolKind::event:     name ="SYMTAB_EVENT"; break;
        case SymbolKind::table:     name ="SYMTAB_TABLE"; break;
    }

    os << name;
    return os;
}

bool hasAddend(RelocationType type)
{
    return type == RelocationType::memoryAddrLeb ||
      type == RelocationType::memoryAddrSleb ||
      type == RelocationType::memoryAddrI32 ||
      type == RelocationType::memoryAddrRelSleb ||
      type == RelocationType::functionOffsetI32 ||
      type == RelocationType::sectionOffsetI32;
}

};
