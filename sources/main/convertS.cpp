// webdass.cpp

#include "Assembler.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>

using namespace libwasm;

static void usage(const char* programName)
{
    std::cerr << "\nUsage: " << programName << " [options] wat_file\n"
         "Options:\n"
         "  -a          generate sequential assmbler file\n"
         "  -c          generate C file\n"
         "  -C          generate optimized C file\n"
         "  -s          generate S-expression assmbler file\n"
         "  -h          print this help message and exit\n"
         "  -o <file>   specify output file.\n"
         "  -S          print statistics\n"
         "\n"
         "Exactly one of '-a', '-s' or '-c' must be given.\n";
}

int main(int argc, char*argv[])
{
    char* p;
    char* parm = nullptr;

    bool wantCCode = false;
    bool wantOptimizedCCode = false;
    bool wantSequential = false;
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
                            std::cerr << "Error: Missing parameter for option " << (p - 1) << '\n';
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
                    wantSequential = true;
                    break;

                case 'C':
                    wantOptimizedCCode = true;
                    // fallthru
                case 'c':
                    wantCCode = true;
                    break;

                case 'h':
                    usage(argv[0]);
                    exit(0);

                case 'o':
                    outputFile = parm;
                    break;

                case 's':
                    wantSequential = false;
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

    if (inputFiles.size() != 1) {
        std::cout << "Error: Exactly one input file must be given.\n";
        usage(argv[0]);
        exit(-1);
    }

    if (outputFile == nullptr) {
        std::cerr << "Error: the '-0' option is required.\n";
        usage(argv[0]);
        exit(-1);
    }

    unsigned errors = 0;
    unsigned warnings = 0;

    for (auto inputFile : inputFiles) {
        std::ifstream inputStream(inputFile, std::ios::binary);

        if (!inputStream.good()) {
            std::cerr << "Error: Unable to open input file '" << inputFile << "'\n";
            exit(1);
        }

        Assembler assembler(inputStream);

        if (assembler.isGood()) {
            assembler.parse();

            std::ofstream outputStream(outputFile);

            if (!outputStream.good()) {
                std::cerr << "Error: Unable to open output file '" << outputFile << "'\n";
                exit(1);
            }

            if (wantCCode) {
                assembler.generateC(outputStream, wantOptimizedCCode);
            } else if (wantSequential) {
                assembler.generate(outputStream);
            } else {
                assembler.generateS(outputStream);
            }

            errors = assembler.getErrorCount();
            warnings = assembler.getWarningCount();

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

