import os
import glob

Decider('MD5-timestamp')

DEBUG = ARGUMENTS.get('DEBUG', '0')
NDEBUG   = ARGUMENTS.get('NDEBUG', '0')
CACHE_DIR = ARGUMENTS.get('CACHE_DIR', '/tmp/scons_cache_dir')
GCC = ARGUMENTS.get('GCC', '0')

CacheDir(CACHE_DIR)

libSources=glob.glob('sources/lib/*.cpp')
mainSources=glob.glob('sources/main/*.cpp')
libCSources=['sources/c/libwasm.c', 'sources/c/spectest.c']

if ('sources/lib/OpcodeTables.cpp' not in libSources):
    libSources.append('sources/lib/OpcodeTables.cpp')

gcc = Environment(CC='gcc',
                  CXX='g++',
                  CXXFLAGS='-std=c++17')

if DEBUG == '1':
    gcc.Append(CCFLAGS=' -g')
else:
    gcc.Append(CCFLAGS=' -g -O3')
    gcc.Append(CXXFLAGS=' -march=native -fomit-frame-pointer -Wall -Wmissing-declarations -Wsign-compare -Wconversion -Wno-sign-conversion -Wold-style-cast -Wno-parentheses')

if NDEBUG == '1':
    gcc.Append(CCFLAGS=' -DNDEBUG')

clang = Environment(CC='clang',
                    CXX='clang++',
                    CXXFLAGS='-std=c++17')

if DEBUG == '1':
    clang.Append(CCFLAGS=' -g')
else:
    clang.Append(CCFLAGS=' -g -O3')
    clang.Append(CXXFLAGS=' -march=native -fomit-frame-pointer -Wall -Wmissing-declarations -Wsign-compare -Wconversion -Wno-sign-conversion -Wold-style-cast -Wno-parentheses -Wno-unused-lambda-capture')

if NDEBUG == '1':
    clang.Append(CCFLAGS=' -DNDEBUG')

if GCC == '1':
    compiler=gcc
else:
    compiler=clang

wasmlib=compiler.Library('lib/libwasm', libSources)

for source in mainSources:
    executable = os.path.split(source)[1]
    executable = executable.replace('.cpp', '')
    if executable == 'makeOpcodeMap':
        continue
    executable = 'bin/' + executable
 
    compiler.Program(executable, source,
            LIBS=['libwasm'], LIBPATH='lib',
            CPPPATH=['.', 'sources/lib'])

Depends('bin/makeOpcodeMap', ['sources/lib/Encodings.h', 'sources/lib/common.h'])

compiler.Program('bin/makeOpcodeMap', ['sources/main/makeOpcodeMap.cpp', 'sources/lib/common.o'],
        CPPPATH=['.', 'sources/lib'])

Command('sources/lib/OpcodeTables.cpp', 'bin/makeOpcodeMap',
        'bin/makeOpcodeMap > sources/lib/OpcodeTables.cpp')

generatedC = ['sources/c/simdFunctions.h', 'sources/c/simdFunctions.cpp']
Depends('lib/libwasmc', generatedC)

Command(generatedC, 'bin/generateSimd',
        'bin/generateSimd sources/c/simdFunctions.h sources/c/simdFunctions.c')

wasmlib=compiler.Library('lib/libwasmc', libCSources)
