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

