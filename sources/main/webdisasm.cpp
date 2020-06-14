// webdisass.cpp

#include "Disassembler.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

using namespace libwasm;

static void usage(const char* programName)
{
    std::cerr << "\nUsage: " << programName << " [options] wasm_files\n"
         "Options:\n"
         "  -a          generate text file\n"
         "  -d          dump raw file content\n"
         "  -h          print this help message and exit\n"
         "  -o <file>   specify output file.\n"
         "  -p          print formatted file content\n"
         "  -P          print formatted file content with disassembled code\n"
         "  -s          generate text file, using S-expressions for code\n"
         "  -S          print statistics\n"
         "\n"
         "The '-a' option cannot be combined with either '-d', '-p' or '-P' options.\n"
         "The '-s' option implies the '-a' option.\n"
         "The '-a' or '-s' option requires an output file and allows only one wasm_file.\n"
         "\n"
         "If the '-o' option is given then only one input file is allowed.\n"
         "\n"
         "For the '-d', '-p' and '-P' options the output file defaults to 'std::cout'.\n";
}

int main(int argc, char*argv[])
{
    char* p;
    char* parm = nullptr;

    bool wantDump = false;
    bool wantGenerate = false;
    bool wantGenerateS = false;
    bool wantShow = false;
    bool wantShowDisassemble = false;
    bool wantStatistics = false;
    const char* outputFile = nullptr;
    std::vector<const char*> inputFiles;

    clock();

    for (int i = 1; i < argc; ++i) {
        p = argv[i];

        if (*p == '-') {
            argv[i] = nullptr;

            parm = nullptr;

            switch (*(++p)) {
                case 'o':
                    if (p[1] != 0) {
                        parm = p + 1;
                    } else {
                        if (i == argc - 1) {
                            std::cerr << "Error: Missing parameter for option " << argv[i] << '\n';
                            usage(argv[0]);
                            exit(-1);
                        }

                        parm = argv[++i];
                        argv[i] = nullptr;
                    }

                    break;

                default:
                    break;
            }

            switch (*(p++)) {
                case 'a':
                    wantGenerate = true;
                    break;

                case 'd':
                    wantDump = true;
                    break;

                case 'h':
                    usage(argv[0]);
                    exit(0);

                case 'o':
                    outputFile = parm;
                    break;

                case 'P':
                    wantShowDisassemble = true;
                    // fallthru

                case 'p':
                    wantShow = true;

                    break;

                case 's':
                    wantGenerateS = true;
                    wantGenerate = true;
                    break;

                case 'S':
                    wantStatistics = true;
                    break;

                default:
                    std::cerr << "Error: Unknown option '" << (p - 1) << "'\n";
                    usage(argv[0]);
                    exit(-1);
            }
        } else {
            inputFiles.push_back(p);
        }
    }

    unsigned errors = 0;
    unsigned warnings = 0;

    if (inputFiles.empty()) {
        std::cout << "Error: no input files specified!\n";
        usage(argv[0]);
        exit(-1);
    }

    if (wantGenerate) {
        if (wantShow || wantDump) {
            std::cerr << "Error: The '-a' or '-s' option can not be used with the '-d','-p' or '-P' options\n";
            usage(argv[0]);
            exit(-1);
        }

        if (outputFile == nullptr) {
            std::cerr << "Error: The '-a' or '-s' option requires an output file\n";
            usage(argv[0]);
            exit(-1);
        }

        if (inputFiles.size() > 1) {
            std::cerr << "Error: The '-a' or '-s' option allows only one input file\n";
            usage(argv[0]);
            exit(-1);
        }
    }

    if (outputFile != nullptr && inputFiles.size() > 1) {
        std::cerr << "Error: When an output file is specified, then only one input file can be given.\n";
        usage(argv[0]);
        exit(-1);
    }

    for (auto inputFile : inputFiles) {
        std::ifstream inputStream(inputFile, std::ios::binary);

        if (!inputStream.good()) {
            std::cerr << "Error: Unable to open input file '" << inputFile << "'\n";
            exit(1);
        }

        Disassembler disassembler(inputStream);

        if (disassembler.isGood()) {
            if (wantShow || wantDump || wantGenerate) {
                if (outputFile != nullptr) {
                    std::ofstream outputStream(outputFile);

                    if (!outputStream.good()) {
                        std::cerr << "Error: Unable to open output file '" << outputFile << "'\n";
                        exit(1);
                    }

                    if (wantShow) {
                        disassembler.show(outputStream, wantShowDisassemble ? 1 : 0);
                    }

                    if (wantDump) {
                        disassembler.dump(outputStream);
                    }

                    if (wantGenerate) {
                        if (wantGenerateS) {
                            disassembler.generateS(outputStream);
                        } else {
                            disassembler.generate(outputStream);
                        }
                    }
                } else {
                    if (wantShow) {
                        disassembler.show(std::cout, wantShowDisassemble ? 1 : 0);
                    }

                    if (wantDump) {
                        disassembler.dump(std::cout);
                    }

                    if (wantGenerate) {
                        disassembler.generate(std::cout);
                    }
                }
            }

            errors = disassembler.getErrorCount();
            warnings = disassembler.getWarningCount();

            if (wantStatistics) {
                std::cout << "Statistic for " << inputFile << ":\n";
                disassembler.getContext().getModule()->getStatistics().show(std::cout, "    ");

                std::cout << '\n';
            }
        } else {
            errors = disassembler.getErrorCount() + 1;
            warnings = disassembler.getWarningCount();
            std::cerr << "Error: Failed to process input file " << inputFile << '\n';
            errors = 1;
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
            double(clock() / double(CLOCKS_PER_SEC)) << "\n";
    }

    return (errors == 0) ? 0 : -1;
}

