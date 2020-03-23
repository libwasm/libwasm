// semantics.cpp

#include "semantics.h"
#include "BackBone.h"

bool checkSemantics(Context& context)
{
    if (context.getDataCountSectionIndex() == invalidSection) {
        auto& sections = context.getSections();
        auto* section = new DataCountSection();

        if (context.getDataSectionIndex() == invalidSection) {
            section->setDataCount(0);
        } else {
            section->setDataCount(uint32_t(context.getDataSection()->getSegments().size()));
        }

        context.setDataCountSectionIndex(sections.size());
        sections.emplace_back(section);
    }

    return true;
}


