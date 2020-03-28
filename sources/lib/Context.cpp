// Context.cpp

#include "Context.h"
#include "BackBone.h"
#include "Encodings.h"

#include <algorithm>
#include <iostream>

bool IndexMap::add(std::string_view id, uint32_t index)
{
    auto it = std::lower_bound(entries.begin(), entries.end(), Entry(id, 0));

    if (it != entries.end() && it->id == id) {
        return false;
    }

    entries.emplace(it, id, index);
    return true;
}

uint32_t IndexMap::getIndex(std::string_view id) const
{
    if (auto it = std::lower_bound(entries.begin(), entries.end(), Entry(id, 0)); it != entries.end() && it->id == id) {
        return it->index;
    }

    return invalidIndex;
}

Context::Context()
{
}

Context::~Context()
{
}

Context::Context(const Context& other) = default;

TypeSection* Context::getTypeSection() const
{
    if (typeSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<TypeSection*>(sections[typeSectionIndex].get());
    }
}

TypeSection* Context::requiredTypeSection()
{
    if (typeSectionIndex == invalidSection) {
        typeSectionIndex = sections.size();
        sections.push_back(std::make_shared<TypeSection>());
    }

    return reinterpret_cast<TypeSection*>(sections[typeSectionIndex].get());
}

ImportSection* Context::getImportSection() const
{
    if (importSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<ImportSection*>(sections[importSectionIndex].get());
    }
}

ImportSection* Context::requiredImportSection()
{
    if (importSectionIndex == invalidSection) {
        importSectionIndex = sections.size();
        sections.push_back(std::make_shared<ImportSection>());
    }

    return reinterpret_cast<ImportSection*>(sections[importSectionIndex].get());
}

MemorySection* Context::getMemorySection() const
{
    if (memorySectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<MemorySection*>(sections[memorySectionIndex].get());
    }
}

MemorySection* Context::requiredMemorySection()
{
    if (memorySectionIndex == invalidSection) {
        memorySectionIndex = sections.size();
        sections.push_back(std::make_shared<MemorySection>());
    }

    return reinterpret_cast<MemorySection*>(sections[memorySectionIndex].get());
}

TableSection* Context::getTableSection() const
{
    if (tableSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<TableSection*>(sections[tableSectionIndex].get());
    }
}

TableSection* Context::requiredTableSection()
{
    if (tableSectionIndex == invalidSection) {
        tableSectionIndex = sections.size();
        sections.push_back(std::make_shared<TableSection>());
    }

    return reinterpret_cast<TableSection*>(sections[tableSectionIndex].get());
}

ElementSection* Context::getElementSection() const
{
    if (elementSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<ElementSection*>(sections[elementSectionIndex].get());
    }
}

ElementSection* Context::requiredElementSection()
{
    if (elementSectionIndex == invalidSection) {
        elementSectionIndex = sections.size();
        sections.push_back(std::make_shared<ElementSection>());
    }

    return reinterpret_cast<ElementSection*>(sections[elementSectionIndex].get());
}

GlobalSection* Context::getGlobalSection() const
{
    if (globalSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<GlobalSection*>(sections[globalSectionIndex].get());
    }
}

GlobalSection* Context::requiredGlobalSection()
{
    if (globalSectionIndex == invalidSection) {
        globalSectionIndex = sections.size();
        sections.push_back(std::make_shared<GlobalSection>());
    }

    return reinterpret_cast<GlobalSection*>(sections[globalSectionIndex].get());
}

ExportSection* Context::getExportSection() const
{
    if (exportSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<ExportSection*>(sections[exportSectionIndex].get());
    }
}

ExportSection* Context::requiredExportSection()
{
    if (exportSectionIndex == invalidSection) {
        exportSectionIndex = sections.size();
        sections.push_back(std::make_shared<ExportSection>());
    }

    return reinterpret_cast<ExportSection*>(sections[exportSectionIndex].get());
}

FunctionSection* Context::getFunctionSection() const
{
    if (functionSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<FunctionSection*>(sections[functionSectionIndex].get());
    }
}

FunctionSection* Context::requiredFunctionSection()
{
    if (functionSectionIndex == invalidSection) {
        functionSectionIndex = sections.size();
        sections.push_back(std::make_shared<FunctionSection>());
    }

    return reinterpret_cast<FunctionSection*>(sections[functionSectionIndex].get());
}

CodeSection* Context::getCodeSection() const
{
    if (codeSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<CodeSection*>(sections[codeSectionIndex].get());
    }
}

CodeSection* Context::requiredCodeSection()
{
    if (codeSectionIndex == invalidSection) {
        codeSectionIndex = sections.size();
        sections.push_back(std::make_shared<CodeSection>());
    }

    return reinterpret_cast<CodeSection*>(sections[codeSectionIndex].get());
}

DataSection* Context::getDataSection() const
{
    if (dataSectionIndex == invalidSection) {
        return nullptr;
    } else {
        return reinterpret_cast<DataSection*>(sections[dataSectionIndex].get());
    }
}

DataSection* Context::requiredDataSection()
{
    if (dataSectionIndex == invalidSection) {
        dataSectionIndex = sections.size();
        sections.push_back(std::make_shared<DataSection>());
    }

    return reinterpret_cast<DataSection*>(sections[dataSectionIndex].get());
}

uint32_t Context::getTypeCount() const
{
    if (auto* section = getTypeSection(); section != nullptr) {
        return uint32_t(section->getTypes().size());
    } else {
        return 0;
    }
}

uint32_t Context::getSegmentCount() const
{
    if (auto* section = getDataSection(); section != nullptr) {
        return uint32_t(section->getSegments().size());
    } else {
        return 0;
    }
}

TypeDeclaration* Context::getType(uint32_t index) const
{
    return getTypeSection()->getTypes()[index].get();
}

void Context::addExport(ExportDeclaration* _export)
{
    auto* section = requiredExportSection();

    section->addExport(_export);
}

void Context::addElement(ElementDeclaration* element)
{
    auto* section = requiredElementSection();

    section->addElement(element);
}

void Context::startFunction()
{
    labelStack.clear();
    localMap.clear();
    localCount = 0;
}

void Context::endFunction()
{
    localMaps.push_back(std::move(localMap));
    localCounts.push_back(localCount);
    labelStack.clear();
    localMap.clear();
    localCount = 0;
}

void Context::startCode(uint32_t number)
{
    localMap = localMaps[number];
    localCount = localCounts[number];
}

void Context::showSections(std::ostream& os, unsigned flags)
{
    for (auto& section : sections) {
        section->show(os, *this, flags);
    }
}

void Context::show(std::ostream& os, unsigned flags)
{
    showSections(os, flags);
}

void Context::generateSections(std::ostream& os)
{
    if (typeSectionIndex != invalidSection) {
        sections[typeSectionIndex]->generate(os, *this);
    }

    if (importSectionIndex != invalidSection) {
        sections[importSectionIndex]->generate(os, *this);
    }

    if (codeSectionIndex != invalidSection) {
        sections[codeSectionIndex]->generate(os, *this);
    }

    if (tableSectionIndex != invalidSection) {
        sections[tableSectionIndex]->generate(os, *this);
    }

    if (memorySectionIndex != invalidSection) {
        sections[memorySectionIndex]->generate(os, *this);
    }

    if (globalSectionIndex != invalidSection) {
        sections[globalSectionIndex]->generate(os, *this);
    }

    if (exportSectionIndex != invalidSection) {
        sections[exportSectionIndex]->generate(os, *this);
    }

    if (startSectionIndex != invalidSection) {
        sections[startSectionIndex]->generate(os, *this);
    }

    if (elementSectionIndex != invalidSection) {
        sections[elementSectionIndex]->generate(os, *this);
    }

    if (dataSectionIndex != invalidSection) {
        sections[dataSectionIndex]->generate(os, *this);
    }
}

void Context::generate(std::ostream& os)
{
    os << "(module";
    generateSections(os);
    os << ")\n";
}

void Context::makeDataCountSection()
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

void BinaryContext::dumpSections(std::ostream& os)
{
    for (auto& section : sections) {
        section->dump(os, *this);
    }
}

void BinaryContext::dump(std::ostream& os)
{
    dumpSections(os);
}

void BinaryContext::writeHeader()
{
    dataBuffer.putU32(wasmMagic);
    dataBuffer.putU32(wasmVersion);
}

void BinaryContext::writeSections()
{
    if (typeSectionIndex != invalidSection) {
        sections[typeSectionIndex]->write(*this);
    }

    if (importSectionIndex != invalidSection) {
        sections[importSectionIndex]->write(*this);
    }

    if (functionSectionIndex != invalidSection) {
        sections[functionSectionIndex]->write(*this);
    }

    if (tableSectionIndex != invalidSection) {
        sections[tableSectionIndex]->write(*this);
    }

    if (memorySectionIndex != invalidSection) {
        sections[memorySectionIndex]->write(*this);
    }

    if (globalSectionIndex != invalidSection) {
        sections[globalSectionIndex]->write(*this);
    }

    if (exportSectionIndex != invalidSection) {
        sections[exportSectionIndex]->write(*this);
    }

    if (startSectionIndex != invalidSection) {
        sections[startSectionIndex]->write(*this);
    }

    if (elementSectionIndex != invalidSection) {
        sections[elementSectionIndex]->write(*this);
    }

    if (dataCountSectionIndex != invalidSection) {
        sections[dataCountSectionIndex]->write(*this);
    }

    if (codeSectionIndex != invalidSection) {
        sections[codeSectionIndex]->write(*this);
    }

    if (dataSectionIndex != invalidSection) {
        sections[dataSectionIndex]->write(*this);
    }
}

void BinaryContext::writeFile(std::ostream& os)
{
    os.write(data().data(), data().size());
}

void BinaryContext::write(std::ostream& os)
{
    dataBuffer.reset();

    writeHeader();
    writeSections();
    writeFile(os);
}

void SourceContext::write(std::ostream& os)
{
    BinaryErrorHandler error;
    BinaryContext bContext(*this, error);

    bContext.data().reset();

    bContext.write(os);
}

bool CheckContext::checkSemantics()
{
    for (auto& section : sections) {
        section->check(*this);
    }

    return errorHandler.getErrorCount() == 0;
}

void CheckContext::checkDataCount(TreeNode* node, uint32_t count)
{
    errorHandler.errorWhen((count != uint32_t(getSegmentCount())), node,
            "Invalid data count ", count, "; expected ", getSegmentCount(), '.');
}

void CheckContext::checkTypeIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(getTypeCount())), node,
            "Type index (", index, ") is larger then maximum (", getTypeCount(), ")");
}

void CheckContext::checkFunctionIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(getFunctionCount())), node,
            "Function index (", index, ") is larger then maximum (", getFunctionCount(), ")");
}

void CheckContext::checkTableIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(getTableCount())), node,
            "Table index (", index, ") is larger then maximum (", getTableCount(), ")");
}

void CheckContext::checkMemoryIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(getMemoryCount())), node,
            "Memory index (", index, ") is larger then maximum (", getMemoryCount(), ")");
}

void CheckContext::checkGlobalIndex(TreeNode* node, uint32_t index)
{
    errorHandler.errorWhen((index >= uint32_t(getGlobalCount())), node,
            "Global index (", index, ") is larger then maximum (", getGlobalCount(), ")");
}

void CheckContext::checkValueType(TreeNode* node, const ValueType& type)
{
    errorHandler.errorWhen((!type.isValid()), node,
            "Invalid valuetype ", int32_t(type));
}

void CheckContext::checkElementType(TreeNode* node, const ElementType& type)
{
    errorHandler.errorWhen((!type.isValid()), node,
            "Invalid element ", int32_t(type));
}

void CheckContext::checkExternalType(TreeNode* node, const ExternalKind& type)
{
    errorHandler.errorWhen((!ExternalKind(type).isValid()), node,
            "Invalid external type ", uint32_t(uint8_t(type)));
}

void CheckContext::checkLimits(TreeNode* node, const Limits& limits)
{
    errorHandler.errorWhen((limits.kind == LimitKind::hasMax && limits.max < limits.min), node,
            "Invalid limits; maximum (", limits.max, ") is less then minimum (", limits.min, ')');
}

void CheckContext::checkMut(TreeNode* node, Mut& mut)
{
    errorHandler.errorWhen((mut != Mut::const_ && mut != Mut::var), node,
            "Invalid mut");
}

