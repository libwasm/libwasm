// Module.cpp

#include "Module.h"
#include "BackBone.h"
#include "Encodings.h"

#include <algorithm>
#include <iostream>

namespace libwasm
{

bool Module::IndexMap::add(std::string_view id, uint32_t index, bool replace)
{
    auto it = std::lower_bound(entries.begin(), entries.end(), Entry(id, 0));

    if (it != entries.end() && it->id == id) {
        if (replace) {
            it->index = index;
        } else {
            return false;
        }
    }

    entries.emplace(it, id, index);
    return true;
}

uint32_t Module::IndexMap::getIndex(std::string_view id) const
{
    if (auto it = std::lower_bound(entries.begin(), entries.end(), Entry(id, 0)); it != entries.end() && it->id == id) {
        return it->index;
    }

    return invalidIndex;
}

uint32_t Module::getLabelIndex(std::string_view id)
{
    uint32_t count = uint32_t(labelStack.size());

    for (uint32_t i = count; i-- > 0; ) {
        if (labelStack[i] == id) {
            return count - i - 1;
        }
    }

    return invalidIndex;
}

bool Module::setTypeSection(TypeSection* section)
{
    if (typeSectionIndex == invalidSection) {
        typeSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setImportSection(ImportSection* section)
{
    if (importSectionIndex == invalidSection) {
        importSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setMemorySection(MemorySection* section)
{
    if (memorySectionIndex == invalidSection) {
        memorySectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setTableSection(TableSection* section)
{
    if (tableSectionIndex == invalidSection) {
        tableSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setElementSection(ElementSection* section)
{
    if (elementSectionIndex == invalidSection) {
        elementSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setGlobalSection(GlobalSection* section)
{
    if (globalSectionIndex == invalidSection) {
        globalSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setExportSection(ExportSection* section)
{
    if (exportSectionIndex == invalidSection) {
        exportSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setFunctionSection(FunctionSection* section)
{
    if (functionSectionIndex == invalidSection) {
        functionSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setCodeSection(CodeSection* section)
{
    if (codeSectionIndex == invalidSection) {
        codeSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setDataSection(DataSection* section)
{
    if (dataSectionIndex == invalidSection) {
        dataSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setDataCountSection(DataCountSection* section)
{
    if (dataCountSectionIndex == invalidSection) {
        dataCountSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

bool Module::setStartSection(StartSection* section)
{
    if (startSectionIndex == invalidSection) {
        startSectionIndex = sections.size();
        sections.emplace_back(section);
        return true;
    } else {
        return false;
    }
}

void Module::addCustomSection(CustomSection* section)
{
    sections.emplace_back(section);
}

TypeSection* Module::getTypeSection() const
{
    if (typeSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<TypeSection*>(sections[typeSectionIndex].get());
    }
}

TypeSection* Module::requiredTypeSection()
{
    if (typeSectionIndex == invalidSection) {
        typeSectionIndex = sections.size();
        sections.push_back(std::make_unique<TypeSection>());
    }

    return reinterpret_cast<TypeSection*>(sections[typeSectionIndex].get());
}

ImportSection* Module::getImportSection() const
{
    if (importSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<ImportSection*>(sections[importSectionIndex].get());
    }
}

ImportSection* Module::requiredImportSection()
{
    if (importSectionIndex == invalidSection) {
        importSectionIndex = sections.size();
        sections.push_back(std::make_unique<ImportSection>());
    }

    return reinterpret_cast<ImportSection*>(sections[importSectionIndex].get());
}

MemorySection* Module::getMemorySection() const
{
    if (memorySectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<MemorySection*>(sections[memorySectionIndex].get());
    }
}

MemorySection* Module::requiredMemorySection()
{
    if (memorySectionIndex == invalidSection) {
        memorySectionIndex = sections.size();
        sections.push_back(std::make_unique<MemorySection>());
    }

    return reinterpret_cast<MemorySection*>(sections[memorySectionIndex].get());
}

TableSection* Module::getTableSection() const
{
    if (tableSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<TableSection*>(sections[tableSectionIndex].get());
    }
}

TableSection* Module::requiredTableSection()
{
    if (tableSectionIndex == invalidSection) {
        tableSectionIndex = sections.size();
        sections.push_back(std::make_unique<TableSection>());
    }

    return reinterpret_cast<TableSection*>(sections[tableSectionIndex].get());
}

ElementSection* Module::getElementSection() const
{
    if (elementSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<ElementSection*>(sections[elementSectionIndex].get());
    }
}

ElementSection* Module::requiredElementSection()
{
    if (elementSectionIndex == invalidSection) {
        elementSectionIndex = sections.size();
        sections.push_back(std::make_unique<ElementSection>());
    }

    return reinterpret_cast<ElementSection*>(sections[elementSectionIndex].get());
}

GlobalSection* Module::getGlobalSection() const
{
    if (globalSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<GlobalSection*>(sections[globalSectionIndex].get());
    }
}

GlobalSection* Module::requiredGlobalSection()
{
    if (globalSectionIndex == invalidSection) {
        globalSectionIndex = sections.size();
        sections.push_back(std::make_unique<GlobalSection>());
    }

    return reinterpret_cast<GlobalSection*>(sections[globalSectionIndex].get());
}

ExportSection* Module::getExportSection() const
{
    if (exportSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<ExportSection*>(sections[exportSectionIndex].get());
    }
}

ExportSection* Module::requiredExportSection()
{
    if (exportSectionIndex == invalidSection) {
        exportSectionIndex = sections.size();
        sections.push_back(std::make_unique<ExportSection>());
    }

    return reinterpret_cast<ExportSection*>(sections[exportSectionIndex].get());
}

FunctionSection* Module::getFunctionSection() const
{
    if (functionSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<FunctionSection*>(sections[functionSectionIndex].get());
    }
}

FunctionSection* Module::requiredFunctionSection()
{
    if (functionSectionIndex == invalidSection) {
        functionSectionIndex = sections.size();
        sections.push_back(std::make_unique<FunctionSection>());
    }

    return reinterpret_cast<FunctionSection*>(sections[functionSectionIndex].get());
}

CodeSection* Module::getCodeSection() const
{
    if (codeSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<CodeSection*>(sections[codeSectionIndex].get());
    }
}

CodeSection* Module::requiredCodeSection()
{
    if (codeSectionIndex == invalidSection) {
        codeSectionIndex = sections.size();
        sections.push_back(std::make_unique<CodeSection>());
    }

    return reinterpret_cast<CodeSection*>(sections[codeSectionIndex].get());
}

DataSection* Module::getDataSection() const
{
    if (dataSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<DataSection*>(sections[dataSectionIndex].get());
    }
}

DataCountSection* Module::getDataCountSection() const
{
    if (dataCountSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<DataCountSection*>(sections[dataCountSectionIndex].get());
    }
}

StartSection* Module::getStartSection() const
{
    if (startSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<StartSection*>(sections[startSectionIndex].get());
    }
}

DataSection* Module::requiredDataSection()
{
    if (dataSectionIndex == invalidSection) {
        dataSectionIndex = sections.size();
        sections.push_back(std::make_unique<DataSection>());
    }

    return reinterpret_cast<DataSection*>(sections[dataSectionIndex].get());
}

uint32_t Module::getTypeCount() const
{
    if (auto* section = getTypeSection(); section != nullptr) {
        return uint32_t(section->getTypes().size());
    } else {
        return 0;
    }
}

uint32_t Module::getImportCount() const
{
    if (auto* section = getImportSection(); section != nullptr) {
        return uint32_t(section->getImports().size());
    } else {
        return 0;
    }
}

uint32_t Module::getSegmentCount() const
{
    if (auto* section = getDataSection(); section != nullptr) {
        return uint32_t(section->getSegments().size());
    } else {
        return 0;
    }
}

TypeDeclaration* Module::getType(uint32_t index) const
{
    return getTypeSection()->getTypes()[index].get();
}

void Module::addTypeEntry(TypeDeclaration* entry)
{
    auto* section = requiredTypeSection();

    section->addType(entry);
}

void Module::addImportEntry(ImportDeclaration* entry)
{
    auto* section = requiredImportSection();

    section->addImport(entry);
}

void Module::addFunctionEntry(FunctionDeclaration* entry)
{
    auto* section = requiredFunctionSection();

    section->addFunction(entry);
}

void Module::addTableEntry(TableDeclaration* entry)
{
    auto* section = requiredTableSection();

    section->addTable(entry);
}

void Module::addMemoryEntry(MemoryDeclaration* entry)
{
    auto* section = requiredMemorySection();

    section->addMemory(entry);
}

void Module::addGlobalEntry(GlobalDeclaration* entry)
{
    auto* section = requiredGlobalSection();

    section->addGlobal(entry);
}

void Module::addDataEntry(DataSegment* entry)
{
    auto* section = requiredDataSection();

    section->addSegment(entry);
}

void Module::addCodeEntry(CodeEntry* entry)
{
    auto* section = requiredCodeSection();

    section->addCode(entry);
}

void Module::addExportEntry(ExportDeclaration* entry)
{
    auto* section = requiredExportSection();

    section->addExport(entry);
}

void Module::addElementEntry(ElementDeclaration* entry)
{
    auto* section = requiredElementSection();

    section->addElement(entry);
}

void Module::startFunction()
{
    labelStack.clear();
    localMap.clear();
    localCount = 0;
}

void Module::endType()
{
    labelStack.clear();
    localMap.clear();
    localCount = 0;
}

void Module::endFunction()
{
    localMaps.push_back(std::move(localMap));
    localCounts.push_back(localCount);
    labelStack.clear();
    localMap.clear();
    localCount = 0;
}

void Module::startCode(uint32_t number)
{
    localMap = localMaps[number];
    localCount = localCounts[number];
}

void Module::showSections(std::ostream& os, unsigned flags)
{
    for (auto& section : sections) {
        section->show(os, this, flags);
    }
}

void Module::show(std::ostream& os, unsigned flags)
{
    showSections(os, flags);
}

void Module::generateSections(std::ostream& os)
{
    if (typeSectionIndex != invalidSection) {
        sections[typeSectionIndex]->generate(os, this);
    }

    if (importSectionIndex != invalidSection) {
        sections[importSectionIndex]->generate(os, this);
    }

    if (codeSectionIndex != invalidSection) {
        sections[codeSectionIndex]->generate(os, this);
    }

    if (tableSectionIndex != invalidSection) {
        sections[tableSectionIndex]->generate(os, this);
    }

    if (memorySectionIndex != invalidSection) {
        sections[memorySectionIndex]->generate(os, this);
    }

    if (globalSectionIndex != invalidSection) {
        sections[globalSectionIndex]->generate(os, this);
    }

    if (exportSectionIndex != invalidSection) {
        sections[exportSectionIndex]->generate(os, this);
    }

    if (startSectionIndex != invalidSection) {
        sections[startSectionIndex]->generate(os, this);
    }

    if (elementSectionIndex != invalidSection) {
        sections[elementSectionIndex]->generate(os, this);
    }

    if (dataSectionIndex != invalidSection) {
        sections[dataSectionIndex]->generate(os, this);
    }
}

void Module::generate(std::ostream& os)
{
    os << "(module";
    generateSections(os);
    os << ")\n";
}

void Module::generateS(std::ostream& os)
{
    useExpressionS = true;
    generate(os);
}

static void makeLoadFunction(std::ostream& os, std::string_view memory,
        std::string_view name, std::string_view cast, std::string_view valueType)
{
    os << "\n" << valueType << " " << name << "(uint64_t offset)"
          "\n{"
          "\n  return " << cast << "(" << memory << ".data + offset);"
          "\n}\n";
}

static void makeStoreFunction(std::ostream& os, std::string_view memory,
        std::string_view name, std::string_view cast, std::string_view valueType)
{
    os << "\nvoid " << name << "(uint64_t offset, " << valueType << " value)"
          "\n{"
          "\n  " << cast << "(" << memory << ".data + offset) = value;"
          "\n}\n";
}

void Module::generateCPreamble(std::ostream& os)
{
    Table* table = nullptr;

    if (auto* tableSection = getTableSection(); tableSection != nullptr) {
        table = tableSection->getTables()[0].get();
    } else if (auto* importSection = getImportSection(); importSection != nullptr) {
        for (auto& import : importSection->getImports()) {
            if (import->getKind() == ExternalType::table) {
                table = static_cast<TableImport*>(import.get());
                break;
            }
        }
    }

    if (table != nullptr) {
        std::string tableName;

        if (!table->getId().empty()) {
            tableName = table->getId();
        } else if (!table->getExternId().empty()) {
            tableName = cName(table->getExternId());
        } else {
            tableName = "table0";
        }

        os << "\n#define TABLE " << tableName << '\n';
    }

    Memory* memory = nullptr;

    if (auto* memorySection = getMemorySection(); memorySection != nullptr) {
        memory = memorySection->getMemories()[0].get();
    } else if (auto* importSection = getImportSection(); importSection != nullptr) {
        for (auto& import : importSection->getImports()) {
            if (import->getKind() == ExternalType::memory) {
                memory = static_cast<MemoryImport*>(import.get());
                break;
            }
        }
    }

    if (memory != nullptr) {
        std::string memoryName;

        if (!memory->getId().empty()) {
            memoryName = memory->getId();
        } else if (!memory->getExternId().empty()) {
            memoryName = cName(memory->getExternId());
        } else {
            memoryName = "memory0";
        }

        os << "\n#define MEMORY " << memoryName << '\n';

        makeLoadFunction(os, memoryName, "loadI8", "*(int8_t*)", "int32_t");
        makeLoadFunction(os, memoryName, "loadU8", "*(uint8_t*)", "uint32_t");
        makeLoadFunction(os, memoryName, "loadI16", "*(int16_t*)", "int32_t");
        makeLoadFunction(os, memoryName, "loadU16", "*(uint16_t*)", "uint32_t");
        makeLoadFunction(os, memoryName, "loadI32", "*(int32_t*)", "int32_t");
        makeLoadFunction(os, memoryName, "loadU32", "*(uint32_t*)", "uint32_t");
        makeLoadFunction(os, memoryName, "loadI64", "*(int64_t*)", "int64_t");
        makeLoadFunction(os, memoryName, "loadF32", "*(float*)", "float");
        makeLoadFunction(os, memoryName, "loadF64", "*(double*)", "double");
        makeLoadFunction(os, memoryName, "loadV128", "*(v128_t*)", "v128_t");

        makeLoadFunction(os, memoryName, "loadI32U8", "(int32_t)*(uint8_t*)", "int32_t");
        makeLoadFunction(os, memoryName, "loadI32I8", "(int32_t)*(int8_t*)", "int32_t");
        makeLoadFunction(os, memoryName, "loadI32U16", "(int32_t)*(uint16_t*)", "int32_t");
        makeLoadFunction(os, memoryName, "loadI32I16", "(int32_t)*(int16_t*)", "int32_t");

        makeLoadFunction(os, memoryName, "loadI64U8", "(int64_t)*(uint8_t*)", "int64_t");
        makeLoadFunction(os, memoryName, "loadI64I8", "(int64_t)*(int8_t*)", "int64_t");
        makeLoadFunction(os, memoryName, "loadI64U16", "(int64_t)*(uint16_t*)", "int64_t");
        makeLoadFunction(os, memoryName, "loadI64I16", "(int64_t)*(int16_t*)", "int64_t");
        makeLoadFunction(os, memoryName, "loadI64U32", "(int64_t)*(uint32_t*)", "int64_t");
        makeLoadFunction(os, memoryName, "loadI64I32", "(int64_t)*(int32_t*)", "int64_t");

        makeStoreFunction(os, memoryName, "storeI32", "*(int32_t*)", "int32_t");
        makeStoreFunction(os, memoryName, "storeI64", "*(int64_t*)", "int64_t");
        makeStoreFunction(os, memoryName, "storeF32", "*(float*)", "float");
        makeStoreFunction(os, memoryName, "storeF64", "*(double*)", "double");
        makeStoreFunction(os, memoryName, "storeV128", "*(v128_t*)", "v128_t");

        makeStoreFunction(os, memoryName, "storeI32I8", "*(int8_t*)", "int32_t");
        makeStoreFunction(os, memoryName, "storeI32I16", "*(int16_t*)", "int32_t");

        makeStoreFunction(os, memoryName, "storeI64I8", "*(int8_t*)", "int64_t");
        makeStoreFunction(os, memoryName, "storeI64I16", "*(int16_t*)", "int64_t");
        makeStoreFunction(os, memoryName, "storeI64I32", "*(int32_t*)", "int64_t");
    }
}

void Module::generateC(std::ostream& os, bool optimized)
{
    os << "\n#include \"libwasm.h\""
          "\n"
          "\n#include <stdint.h>"
          "\n#include <math.h>"
          "\n#include <string.h>"
          "\n";

    if (auto* typeSection = getTypeSection(); typeSection != nullptr) {
        typeSection->generateC(os, this);
        os << '\n';
    }

    if (auto* importSection = getImportSection(); importSection != nullptr) {
        importSection->generateC(os, this);
        os << '\n';
    }

    if (auto* globalSection = getGlobalSection(); globalSection != nullptr) {
        globalSection->generateC(os, this);
        os << '\n';
    }

    if (auto* memorySection = getMemorySection(); memorySection != nullptr) {
        memorySection->generateC(os, this);
    }

    if (auto* tableSection = getTableSection(); tableSection != nullptr) {
        tableSection->generateC(os, this);
    }

    generateCPreamble(os);

    if (auto* functionSection = getFunctionSection(); functionSection != nullptr) {
        functionSection->generateC(os, this);
        os << '\n';
    }

    if (auto* codeSection = getCodeSection(); codeSection != nullptr) {
        codeSection->generateC(os, this, optimized);
        os << '\n';
    }
}

void Module::makeDataCountSection()
{
    if (dataCountSectionIndex == invalidSection) {
        auto* section = new DataCountSection();

        if (dataSectionIndex == invalidSection) {
            section->setDataCount(0);
        } else {
            section->setDataCount(uint32_t(getDataSection()->getSegments().size()));
        }

        dataCountSectionIndex = sections.size();
        sections.emplace_back(section);
    }
}

Module::Statistics Module::getStatistics()
{
    Statistics result;

    result.elementCount = getElementCount();
    result.eventCount = getEventCount();
    result.exportCount = getExportCount();
    result.functionCount = getFunctionCount();
    result.globalCount = getGlobalCount();
    result.importCount = getImportCount();
    result.memoryCount = getMemoryCount();
    result.tableCount = getTableCount();
    result.typeCount = getTypeCount();
    result.dataSegmentCount = getSegmentCount();
    result.sectionCount = uint32_t(sections.size());

    if (auto* codeSection = getCodeSection(); codeSection != nullptr) {
        for (auto& code : codeSection->getCodes()) {
            result.instructionCount += code->getExpression()->getInstructions().size();
        }
    }

    if (auto* globalSection = getGlobalSection(); globalSection != nullptr) {
        for (auto& global : globalSection->getGlobals()) {
            result.initInstructionCount += global->getExpression()->getInstructions().size();
        }
    }

    if (auto* elementSection = getElementSection(); elementSection != nullptr) {
        for (auto& element : elementSection->getElements()) {
            if (element->getExpression() != 0) {
                result.initInstructionCount += element->getExpression()->getInstructions().size();
            }

            for (auto& expression : element->getRefExpressions()) {
                result.initInstructionCount += expression->getInstructions().size();
            }
        }
    }

    if (auto* dataSection = getDataSection(); dataSection != nullptr) {
        for (auto& data : dataSection->getSegments()) {
            if (auto* expression = data->getExpression(); expression != nullptr) {
                result.initInstructionCount += expression->getInstructions().size();
            }
        }
    }

    return result;
}

void Module::Statistics::show(std::ostream& os, std::string_view indent)
{
    os << indent << "Number of sections         " << sectionCount << '\n';
    os << indent << "Number of types            " << typeCount << '\n';
    os << indent << "Number of imports          " << importCount << '\n';
    os << indent << "Number of functions        " << functionCount << '\n';
    os << indent << "Number of tables           " << tableCount << '\n';
    os << indent << "Number of memories         " << memoryCount << '\n';
    os << indent << "Number of globals          " << globalCount << '\n';
    os << indent << "Number of exports          " << exportCount << '\n';
    os << indent << "Number of elements         " << elementCount << '\n';
    os << indent << "Number of data segments    " << dataSegmentCount << '\n';
    os << indent << "Number of instructions     " << (instructionCount + initInstructionCount) << '\n';
    os << indent << "              in code      " << instructionCount << '\n';
    os << indent << "              in inits     " << initInstructionCount << '\n';
}

};
