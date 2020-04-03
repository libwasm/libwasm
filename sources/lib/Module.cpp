// Module.cpp

#include "Module.h"
#include "BackBone.h"
#include "Encodings.h"

#include <algorithm>
#include <iostream>

namespace libwasm
{

bool Module::IndexMap::add(std::string_view id, uint32_t index)
{
    auto it = std::lower_bound(entries.begin(), entries.end(), Entry(id, 0));

    if (it != entries.end() && it->id == id) {
        return false;
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

};
