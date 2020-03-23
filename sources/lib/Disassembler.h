// disassemble.h

#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H

#include "BackBone.h"
#include "Context.h"
#include "DataBuffer.h"
#include "Encodings.h"
#include "Instruction.h"
#include "common.h"

#include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

class Disassembler
{
    public:
        Disassembler(std::istream& stream)
          : context(data, msgs, sections)
        {
            good = readWasm(stream);
        }

        ~Disassembler() = default;

        bool isGood() const
        {
            return good;
        }

        void dump(std::ostream& os);
        void show(std::ostream& os, unsigned flags);
        void generate(std::ostream& os, unsigned flags);

    private:
        bool readWasm(std::istream& stream);
        bool readFile(std::istream& stream);
        bool checkHeader();
        bool readSections();

        void dumpSections(std::ostream& os);
        void showSections(std::ostream& os, unsigned flags);
        void generateSections(std::ostream& os, unsigned flags);

        DataBuffer data;
        BinaryErrorHandler msgs;

        bool good = false;

        BinaryContext context;
        std::vector<std::unique_ptr<Section>> sections;
};


#endif
