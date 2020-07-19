// wasmasm.cpp

#include "Assembler.h"
#include "Disassembler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

using namespace libwasm;

enum class Option: char
{
    text = 't',
    sText = 'T',
    binary = 'b',
    c = 'c',
    optimizedC = 'C',
    dump = 'd',
    print = 'p',
    printAssembler = 'P',
};

static std::map<std::string, std::vector<Option>> options;
static unsigned errors = 0;
static unsigned warnings = 0;

static void usage(const char* programName)
{
    std::cerr << "\nUsage: " << programName << " <input_file> [options]"
         "\nOptions:"
         "\n  -b <output_file>   generate binary file"
         "\n  -c [output_file]   generate C file"
         "\n  -C [output_file]   generate optimized C file"
         "\n  -d [output_file]   dump raw file content"
         "\n  -h                 print this help message and exit"
         "\n  -p [output_file]   print formatted file content"
         "\n  -P [output_file]   print formatted file content with dassembled code"
         "\n  -S                 print statistics"
         "\n  -t [output_file]   generate text file"
         "\n  -T [output_file]   generate text file, using S-expressions for code\n"
         "\n"
         "\nThe input file can be a text, binary or script file."
         "\nThe input file must precede all options."
         "\n"
         "\nFor the '-b' command, the output file is required."
         "\nFor all other options, the output file defaults to std::cout."
         "\nThe '-d' option only applies for a binary input file."
         "\nWhen the input file is a script, then only the '-c' and '-C' options apply."
         "\n"
         "\n";
}

static void generate(std::ostream& os, Module* module, Option option)
{
    switch (option) {
        case Option::text:
            module->generate(os);
            break;

        case Option::sText:
            module->generateS(os);
            break;

        case Option::binary:
            module->write(os);
            break;

        case Option::c:
            module->generateC(os, false);
            break;

        case Option::optimizedC:
            module->generateC(os, true);
            break;

        case Option::dump:
            module->dump(os);
            break;

        case Option::print:
            module->show(os, 0);
            break;

        case Option::printAssembler:
            module->show(os, 1);
            break;

        default:
            break;
    }
}

static void generate(Module* module, bool (*predicate)(const Option& option))
{
    for (const auto& option : options) {
        const auto& fileName = option.first;
        const auto& wants = option.second;

        if (auto it = std::find_if(wants.begin(), wants.end(), predicate); it != wants.end()) {
            if (fileName.empty()) {
                for (; it != wants.end(); ++it, it = std::find_if(it,  wants.end(), predicate)) {
                    generate(std::cout, module, *it);
                }
            } else if (std::ofstream os(fileName.c_str()); os.good()) {
                for (; it != wants.end(); ++it, it = std::find_if(it,  wants.end(), predicate)) {
                    generate(os, module, *it);
                }
            } else {
                std::cerr << "Error: Unable to open output file '" << fileName << "'\n";
                ++errors;
            }
        }
    }
}

static bool isCOption(const Option& option)
{
    if (option == Option::c || option == Option::optimizedC) {
        return true;
    } else {
        std::cerr << "Warning: option '-" << char(option) << "' ignored.\n";
        return false;
    }
}

static bool isText(const Option& option)
{
    if (option == Option::dump) {
        std::cerr << "Warning: option '-" << char(option) << "' ignored.\n";
        return false;
    } else {
        return true;
    }
}

static bool isBinary(const Option& option)
{
    return  true;
}

static void generateC(Script* script)
{
    for (const auto& option : options) {
        const auto& fileName = option.first;
        const auto& wants = option.second;

        if (auto it = std::find_if(wants.begin(), wants.end(), isCOption); it != wants.end()) {
            if (fileName.empty()) {
                for (; it != wants.end(); ++it, it = std::find_if(it,  wants.end(), isCOption)) {
                    script->generateC(std::cout, *it == Option::optimizedC);
                }
            } else if (std::ofstream os(fileName.c_str()); os.good()) {
                for (; it != wants.end(); ++it, it = std::find_if(it,  wants.end(), isCOption)) {
                    script->generateC(os, *it == Option::optimizedC);
                }
            } else {
                std::cerr << "Error: Unable to open output file '" << fileName << "'\n";
                ++errors;
            }
        }
    }
}

int main(int argc, char*argv[])
{
    const char* inputFile = nullptr;

    bool wantStatistics = false;

    clock();

    if (argc > 1 && *argv[1] != '-') {
        inputFile = argv[1];
    } else if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        usage(argv[0]);
        exit(0);
    } else {
        std::cerr << "Error: Missing input file.\n";
        errors++;
    }

    for (int i = 2; i < argc; ++i) {
        const char* p = argv[i];

        if (*p == '-') {
            switch (*(++p)) {
                case 'b':
                    if (p[1] != 0) {
                        options[p + 1].push_back(Option(*p));
                    } else if (i != argc - 1 && *argv[i + 1] != '-') {
                        options[argv[++i]].push_back(Option(*p));
                    } else {
                        std::cerr << "Error: Missing parameter for option " << (p - 1) << '\n';
                        errors++;
                    }

                    break;

                case 'c':
                case 'C':
                case 'd':
                case 'p':
                case 'P':
                case 't':
                case 'T':
                    if (p[1] != 0) {
                        options[p + 1].push_back(Option(*p));
                    } else if (i != argc - 1 && *argv[i + 1] != '-') {
                        options[argv[++i]].push_back(Option(*p));
                    } else {
                        options[""].push_back(Option(*p));
                    }

                    break;

                case 'h':
                    usage(argv[0]);
                    exit(0);

                case 'S':
                    wantStatistics = true;
                    break;

                default:
                    std::cerr << "Error: Unknown option '" << (p - 1) << "'\n";
                    usage(argv[0]);
                    exit(-1);
            }
        } else {
            if (inputFile != nullptr) {
                std::cerr << "Error: Only one input file can be specified.\n";
                errors++;
            } else {
                inputFile = p;
            }
        }
    }

    if (errors > 0) {
        usage(argv[0]);
        exit(-1);
    }

    std::ifstream inputStream(inputFile, std::ios::binary);

    if (!inputStream.good()) {
        std::cerr << "Error: Unable to open input file '" << inputFile << "'\n";
        exit(1);
    }

    bool binary = isBinary(inputStream);

    if (binary) {
        if (Disassembler disassembler(inputStream); disassembler.isGood()) {
            generate(disassembler.getModule().get(), isBinary);

            if (wantStatistics) {
                std::cout << "Statistic for " << inputFile << ":\n";
                disassembler.getContext().getModule()->getStatistics().show(std::cout, "    ");

                std::cout << '\n';
            }
        } else {
            errors = disassembler.getErrorCount() + 1;
            warnings = disassembler.getWarningCount();
            std::cerr << "Error: Failed to process input file " << inputFile << '\n';
        }
    } else {
        if (Assembler assembler(inputStream); assembler.isGood()) {
            assembler.parse();

            if (assembler.isScript()) {
                generateC(assembler.getScript());
            } else {
                generate(assembler.getModule().get(), isText);
            }

            if (wantStatistics) {
                std::cout << "Statistic for " << inputFile << ":\n";
                assembler.getContext().getModule()->getStatistics().show(std::cout, "    ");

                std::cout << '\n';
            }
        } else {
            errors = assembler.getErrorCount() + 1;
            warnings = assembler.getWarningCount();
            std::cerr << "Error: Failed to process input file " << inputFile << '\n';
        }
    }

    if (wantStatistics || errors != 0 || warnings != 0) {
        if (errors == 0) {
            std::cout << "NO ERRORS; ";
        } else if (errors == 1) {
            std::cout << "1 ERROR; ";
        } else {
            std::cout << errors << " ERRORS; ";
        }

        if (warnings == 0) {
            std::cout << "NO WARNINGS; ";
        } else if (warnings == 1) {
            std::cout << "1 WARNING; ";
        } else {
            std::cout << warnings << " WARNINGS; ";
        }

        std::cout << '\n';
    }

    if (wantStatistics) {
        std::cout << "CPU time = " << std::setw(4) << std::setprecision(2) << std::fixed <<
            (double(clock()) / double(CLOCKS_PER_SEC)) << "\n";
    }

    return (errors == 0) ? 0 : -1;
}
