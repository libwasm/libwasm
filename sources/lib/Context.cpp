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
        sections.push_back(std::make_unique<ExportSection>());
    }

    auto* section = getExportSection();

    section->addExport(_export);
}

void Context::addElement(ElementDeclaration* element)
{
    if (getElementSectionIndex() == invalidSection) {
        setElementSectionIndex(sections.size());
        sections.push_back(std::make_unique<ElementSection>());
    }

    auto* section = getElementSection();

    section->addElement(element);
}
