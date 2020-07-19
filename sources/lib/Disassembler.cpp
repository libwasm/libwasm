// disassemble.cpp

#include "Disassembler.h"

namespace libwasm
{

bool Disassembler::readFile(std::istream& stream)
{
    return data.readFile(stream);
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
    context.setModule(module.get());

    while (!data.atEnd()) {
        auto c = data.getU8();

        switch (c) {
            case SectionType::custom:
                module->addCustomSection(CustomSection::read(context));
                break;

            case SectionType::type:
                if (!module->setTypeSection(TypeSection::read(context))) {
                    msgs.error("More than 1 type section is given.");
                }

                break;

            case SectionType::import:
                if (!module->setImportSection(ImportSection::read(context))) {
                    msgs.error("More than 1 import section is given.");
                }

                break;

            case SectionType::function:
                if (!module->setFunctionSection(FunctionSection::read(context))) {
                    msgs.error("More than 1 function section is given.");
                }

                break;

            case SectionType::table:
                if (!module->setTableSection(TableSection::read(context))) {
                    msgs.error("More than 1 table section is given.");
                }

                break;

            case SectionType::memory:
                if (!module->setMemorySection(MemorySection::read(context))) {
                    msgs.error("More than 1 memory section is given.");
                }

                break;

            case SectionType::global:
                if (!module->setGlobalSection(GlobalSection::read(context))) {
                    msgs.error("More than 1 global section is given.");
                }

                break;

            case SectionType::export_:
                if (!module->setExportSection(ExportSection::read(context))) {
                    msgs.error("More than 1 ecport section is given.");
                }

                break;

            case SectionType::start:
                if (!module->setStartSection(StartSection::read(context))) {
                    msgs.error("More than 1 start section is given.");
                }

                break;

            case SectionType::element:
                if (!module->setElementSection(ElementSection::read(context))) {
                    msgs.error("More than 1 element section is given.");
                }

                break;

            case SectionType::code:
                if (!module->setCodeSection(CodeSection::read(context))) {
                    msgs.error("More than 1 code section is given.");
                }

                break;

            case SectionType::data:
                if (!module->setDataSection(DataSection::read(context))) {
                    msgs.error("More than 1 data section is given.");
                }

                break;

            case SectionType::dataCount:
                if (!module->setDataCountSection(DataCountSection::read(context))) {
                    msgs.error("More than 1 data count section is given.");
                }

                break;

            default:
                msgs.error("Invalid section opcode ", unsigned(c));
                return false;
        }
    }

    return msgs.getErrorCount() == 0;
}

bool Disassembler::checkSemantics()
{
    if (context.getModule()->needsDataCount()) {
        context.getModule()->makeDataCountSection();
    }

    CheckErrorHandler error(msgs.getErrorStream());
    CheckContext checkContext(context, error);

    if (!checkContext.checkSemantics()) {
        errorCount += checkContext.msgs().getErrorCount();
        warningCount += checkContext.msgs().getWarningCount();
        return false;
    } else {
        return true;
    }
}

bool Disassembler::readWasm(std::istream& stream)
{
    return readFile(stream) &&
        checkHeader() &&
        readSections() &&
        checkSemantics();

    return true;
}

};
