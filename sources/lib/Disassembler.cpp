// disassemble.cpp

#include "Disassembler.h"
#include "semantics.h"

bool Disassembler::readFile(std::istream& stream)
{
    stream.seekg(0, std::ios::end);

    size_t fileSize = stream.tellg();

    stream.seekg(0, std::ios::beg);

    data.resize(fileSize);
    stream.read(data.data(), fileSize);
    return (size_t(stream.gcount()) == fileSize);
}

bool Disassembler::checkHeader()
{
    if (auto m = data.getU32(); m != wasmMagic) {
        context.msgs().error("Invalid magic number ",std::hex, m, std::dec);
        return false;
    }

    if (auto v = data.getU32(); v != wasmVersion) {
        context.msgs().error("Invalid version number ", v);
        return false;
    }

    return true;
}

bool Disassembler::readSections()
{
    while (!data.atEnd()) {
        auto c = data.getU8();

        switch (c) {
            case SectionType::custom:
                {
                    context.customSectionIndexes.push_back(sections.size());
                    sections.emplace_back(CustomSection::read(context));
                    break;
                }

            case SectionType::type:
                {
                    auto* section = new TypeSection;

                    context.typeSectionIndex = sections.size();
                    sections.emplace_back(section);
                    TypeSection::read(context, section);
                    break;
                }

            case SectionType::import:
                {
                    context.importSectionIndex = sections.size();
                    sections.emplace_back(ImportSection::read(context));
                    break;
                }

            case SectionType::function:
                {
                    context.functionSectionIndex = sections.size();
                    sections.emplace_back(FunctionSection::read(context));
                    break;
                }

            case SectionType::table:
                {
                    context.tableSectionIndex = sections.size();
                    sections.emplace_back(TableSection::read(context));
                    break;
                }

            case SectionType::memory:
                {
                    context.memorySectionIndex = sections.size();
                    sections.emplace_back(MemorySection::read(context));
                    break;
                }

            case SectionType::global:
                {
                    context.globalSectionIndex = sections.size();
                    sections.emplace_back(GlobalSection::read(context));
                    break;
                }

            case SectionType::export_:
                {
                    context.exportSectionIndex = sections.size();
                    sections.emplace_back(ExportSection::read(context));
                    break;
                }

            case SectionType::start:
                {
                    context.startSectionIndex = sections.size();
                    sections.emplace_back(StartSection::read(context));
                    break;
                }

            case SectionType::element:
                {
                    context.elementSectionIndex = sections.size();
                    sections.emplace_back(ElementSection::read(context));
                    break;
                }

            case SectionType::code:
                {
                    context.codeSectionIndex = sections.size();
                    sections.emplace_back(CodeSection::read(context));
                    break;
                }

            case SectionType::data:
                {
                    context.dataSectionIndex = sections.size();
                    sections.emplace_back(DataSection::read(context));
                    break;
                }

            case SectionType::dataCount:
                {
                    context.dataCountSectionIndex = sections.size();
                    sections.emplace_back(DataCountSection::read(context));
                    break;
                }

            default:
                context.msgs().error("Invalid section opcode ", unsigned(c));
                return false;
        }
    }

    return true;
}

bool Disassembler::readWasm(std::istream& stream)
{
    return readFile(stream) &&
        checkHeader() &&
        readSections() &&
        checkSemantics(context);

    return true;
}

void Disassembler::showSections(std::ostream& os, unsigned flags = 0)
{
    for (auto& section : sections) {
        section->show(os, context, flags);
    }
}

void Disassembler::generateSections(std::ostream& os, unsigned flags = 0)
{
    if (context.typeSectionIndex != invalidSection) {
        context.sections[context.typeSectionIndex]->generate(os, context, flags);
    }

    if (context.importSectionIndex != invalidSection) {
        context.sections[context.importSectionIndex]->generate(os, context, flags);
    }

    if (context.codeSectionIndex != invalidSection) {
        context.sections[context.codeSectionIndex]->generate(os, context, flags);
    }

    if (context.tableSectionIndex != invalidSection) {
        context.sections[context.tableSectionIndex]->generate(os, context, flags);
    }

    if (context.memorySectionIndex != invalidSection) {
        context.sections[context.memorySectionIndex]->generate(os, context, flags);
    }

    if (context.globalSectionIndex != invalidSection) {
        context.sections[context.globalSectionIndex]->generate(os, context, flags);
    }

    if (context.exportSectionIndex != invalidSection) {
        context.sections[context.exportSectionIndex]->generate(os, context, flags);
    }

    if (context.startSectionIndex != invalidSection) {
        context.sections[context.startSectionIndex]->generate(os, context, flags);
    }

    if (context.elementSectionIndex != invalidSection) {
        context.sections[context.elementSectionIndex]->generate(os, context, flags);
    }

    if (context.dataSectionIndex != invalidSection) {
        context.sections[context.dataSectionIndex]->generate(os, context, flags);
    }
}

void Disassembler::dumpSections(std::ostream& os)
{
    for (auto& section : sections) {
        section->dump(os, context);
    }
}

void Disassembler::dump(std::ostream& os)
{
    dumpSections(os);
}

void Disassembler::show(std::ostream& os, unsigned flags)
{
    showSections(os, flags);
}

void Disassembler::generate(std::ostream& os, unsigned flags)
{
    os << "(module";
    generateSections(os, flags);
    os << ")\n";
}

