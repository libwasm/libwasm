#include "Encodings.h"

#include <vector>

#ifndef MAKEOPCODEMAP_CPP
#define MAKEOPCODEMAP_CPP

using namespace libwasm;

std::string opcodeName(std::string_view name)
{
    std::string result;

    if (name == "if" || name == "else" || name == "try" || name == "catch" ||
            name == "throw" || name == "rethrow" || name == "return") {
        result = name;
        result += '_';
    } else {
        for (auto c : name) {
            if (c == '.') {
                result += "__";
            } else {
                result += c;
            }
        }
    }

    return result;
}

int main()
{
    auto* info = Opcode::getInfoTable();
    const unsigned bucketCount = 512;

    std::cout << "\n#include \"Encodings.h\"";
    std::cout << "\nnamespace libwasm"
        "\n{";

    {
        std::vector<Opcode::NameEntry> buckets[bucketCount];

        unsigned opcode = ~0u;

        for (size_t i = 0; ; ++i) {
            if (opcode != ~0u && info[i].opcode < opcode) {
                std::cerr << "Opcode info not sorted at entry " << info[i].name << std::endl;
                exit(1);
            }

            opcode = info[i].opcode;

            auto index = hash(info[i].name) % bucketCount;

            buckets[index].emplace_back(info[i].name, opcode);
            if (info[i].opcode == Opcode::max) {
                break;
            }
        }

        std::vector<Opcode::Index> indexes;
        std::vector<Opcode::NameEntry> entries;

        for (const auto& bucket : buckets) {
            indexes.emplace_back(unsigned(entries.size()), unsigned(bucket.size()));

            for (auto& entry : bucket) {
                entries.push_back(entry);
            }
        }

        std::cout <<
            "\nOpcode::Index const Opcode::nameIndexes[" << bucketCount << "] = {";

        int count = 8;

        for (const auto& index : indexes) {
            if (count++ == 8) {
                std::cout << "\n   ";
                count = 1;
            }

            std::cout << " { " << index.index << ", " << index.size << " },";
        }

        std::cout <<
            "\n};"
            "\n";

        std::cout <<
            "\nOpcode::NameEntry const Opcode::nameEntries [" << entries.size() << "] = {";

        for (const auto& entry : entries) {
            std::cout << "\n    { \"" << entry.name << "\", Opcode::" << opcodeName(entry.name) << " },";
        }

        std::cout <<
            "\n};"
            "\n";
    }

    {
        std::vector<Opcode::OpcodeEntry> buckets[bucketCount];

        for (size_t i = 0; ; ++i) {
            Opcode opcode = info[i].opcode;

            auto index = opcode.hash() % bucketCount;

            buckets[index].emplace_back(opcode, unsigned(i));
            if (info[i].opcode == Opcode::max) {
                break;
            }
        }

        std::vector<Opcode::Index> indexes;
        std::vector<Opcode::OpcodeEntry> entries;

        for (const auto& bucket : buckets) {
            indexes.emplace_back(unsigned(entries.size()), unsigned(bucket.size()));

            for (auto& entry : bucket) {
                entries.push_back(entry);
            }
        }

        std::cout <<
            "\nOpcode::Index const Opcode::opcodeIndexes[" << bucketCount << "] = {";

        int count = 8;

        for (const auto& index : indexes) {
            if (count++ == 8) {
                std::cout << "\n   ";
                count = 1;
            }

            std::cout << " { " << index.index << ", " << index.size << " },";
        }

        std::cout <<
            "\n};"
            "\n";

        std::cout <<
            "\nOpcode::OpcodeEntry const Opcode::opcodeEntries [" << entries.size() << "] = {";

        for (const auto& entry : entries) {
            std::cout << "\n    { 0x" << std::hex << entry.opcode << std::dec << ", " << entry.infoIndex << " },";
        }

        std::cout <<
            "\n};"
            "\n";
    }

    std::cout <<
        "\n};"
        "\n";
}

#endif
