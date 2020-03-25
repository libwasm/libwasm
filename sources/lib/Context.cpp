// Context.cpp

#include "Context.h"
#include "BackBone.h"

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

uint32_t Context::getTypeCount() const
{
    if (auto* section = getTypeSection(); section != nullptr) {
        return uint32_t(section->getTypes().size());
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
    if (getExportSectionIndex() == invalidSection) {
        setExportSectionIndex(sections.size());
        sections.push_back(std::make_shared<ExportSection>());
    }

    auto* section = getExportSection();

    section->addExport(_export);
}

void Context::addElement(ElementDeclaration* element)
{
    if (getElementSectionIndex() == invalidSection) {
        setElementSectionIndex(sections.size());
        sections.push_back(std::make_shared<ElementSection>());
    }

    auto* section = getElementSection();

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

void Context::generateSections(std::ostream& os, unsigned flags)
{
    if (typeSectionIndex != invalidSection) {
        sections[typeSectionIndex]->generate(os, *this, flags);
    }

    if (importSectionIndex != invalidSection) {
        sections[importSectionIndex]->generate(os, *this, flags);
    }

    if (codeSectionIndex != invalidSection) {
        sections[codeSectionIndex]->generate(os, *this, flags);
    }

    if (tableSectionIndex != invalidSection) {
        sections[tableSectionIndex]->generate(os, *this, flags);
    }

    if (memorySectionIndex != invalidSection) {
        sections[memorySectionIndex]->generate(os, *this, flags);
    }

    if (globalSectionIndex != invalidSection) {
        sections[globalSectionIndex]->generate(os, *this, flags);
    }

    if (exportSectionIndex != invalidSection) {
        sections[exportSectionIndex]->generate(os, *this, flags);
    }

    if (startSectionIndex != invalidSection) {
        sections[startSectionIndex]->generate(os, *this, flags);
    }

    if (elementSectionIndex != invalidSection) {
        sections[elementSectionIndex]->generate(os, *this, flags);
    }

    if (dataSectionIndex != invalidSection) {
        sections[dataSectionIndex]->generate(os, *this, flags);
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

void Context::generate(std::ostream& os, unsigned flags)
{
    os << "(module";
    generateSections(os, flags);
    os << ")\n";
}


