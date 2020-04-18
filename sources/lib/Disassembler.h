// disassemble.h

#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#include "BackBone.h"
#include "Context.h"
#include "DataBuffer.h"
#include "Encodings.h"
#include "Instruction.h"
#include "Module.h"
#include "common.h"

#include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace libwasm
{
class Disassembler
{
    public:
        Disassembler(std::istream& stream)
          : context(msgs), data(context.data()), module(context.getModule())
        {
            good = readWasm(stream);
        }

        Disassembler(std::istream& stream, std::ostream& errorStream)
          : msgs(errorStream), context(msgs), data(context.data()), module(context.getModule())
        {
            good = readWasm(stream);
        }

        ~Disassembler() = default;

        bool isGood() const
        {
            return good;
        }

        void dump(std::ostream& os)
        {
            context.dump(os);
        }

        void show(std::ostream& os, unsigned flags)
        {
            module->show(os, flags);
        }

        void generate(std::ostream& os)
        {
            module->generate(os);
        }

        void write(std::ostream& os)
        {
            context.write(os);
        }

        auto getErrorCount() const
        {
            return errorCount + msgs.getErrorCount();
        }

        auto getWarningCount() const
        {
            return warningCount + msgs.getWarningCount();
        }

    private:
        bool readWasm(std::istream& stream);
        bool readFile(std::istream& stream);
        bool checkHeader();
        bool readSections();
        bool checkSemantics();

        unsigned errorCount = 0;
        unsigned warningCount = 0;

        void dumpSections(std::ostream& os);
        void generateSections(std::ostream& os, unsigned flags);

        BinaryErrorHandler msgs;

        bool good = false;

        BinaryContext context;
        DataBuffer& data;
        Module* module = 0;
};
};


#endif
