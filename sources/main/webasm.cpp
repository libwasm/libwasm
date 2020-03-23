// webdass.cpp

#include "Assembler.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

static void usage(const char* programName)
{
    std::cerr << "\nUsage: " << programName << " [options] wasm_files\n"
         "Options:\n"
         "  -a          generate binary file\n"
         "  -d          dump raw file content\n"
         "  -h          print this help message and exit\n"
         "  -o <file>   specify output file.\n"
         "  -p          print formatted file content\n"
         "  -P          print formatted file content with dassembled code\n:\n"
         "  -S          print statistics\n"
         "\n"
         "The 'a' option cannot be combined with either 'd', 'p' or 'P' options.\n"
         "\n"
         "If the 'o' option is given then only one input file is allowed.\n"
         "\n"
         "For the 'd', 'p' and 'P' options the output file defaults to 'std::cout'.\n"
         "For the 'a' option the aoutput file defaults to the input file name with\n"
         "the extension replaced with '.wat'.\n";
}

int main(int argc, char*argv[])
{
    char* p;
    char* parm = nullptr;

    bool wantDump = false;
    bool wantAsm = false;
    bool wantShow = false;
    bool wantShowDassemble = false;
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
                    wantAsm = true;
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
                    wantShowDassemble = true;
                    // fallthru

                case 'p':
                    wantShow = true;

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

    if (inputFiles.empty()) {
        std::cout << "Error: no input files specified!\n";
        usage(argv[0]);
        exit(-1);
    }

    if (wantAsm) {
        if (wantShow || wantDump) {
            std::cerr << "Error: The 'a' options can not be used with the 'd','p' or 'P' options\n";
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

        Assembler assembler(inputStream);

        if (assembler.isGood()) {
            if (wantShow || wantDump || wantAsm) {
                if (outputFile != nullptr) {
                    std::ofstream outputStream(outputFile);

                    if (!outputStream.good()) {
                        std::cerr << "Error: Unable to open output file '" << outputFile << "'\n";
                        exit(1);
                    }

                    if (wantShow) {
                        assembler.show(outputStream, wantShowDassemble ? 1 : 0);
                    }

                    if (wantDump) {
                        assembler.dump(outputStream);
                    }

                    if (wantAsm) {
                        assembler.write(outputStream);
                    }
                } else {
                    if (wantShow) {
                        assembler.show(std::cout, wantShowDassemble ? 1 : 0);
                    }

                    if (wantDump) {
                        assembler.dump(std::cout);
                    }

                    if (wantAsm) {
                        assembler.write(std::cout);
                    }
                }
            }
        } else {
            std::cerr << "Error: Failed to process input file " << inputFile << '\n';
        }
    }

    if (wantStatistics) {
        std::cout << "CPU time = " << std::setw(4) << std::setprecision(2) << std::fixed <<
            double(clock() / double(CLOCKS_PER_SEC)) << "\n";
    }
}

