# libwasm

Linwasm is a library to manipulate webassembly files from C++.
It also contains an assembler (converting .wat to .wasm), a dissassembler (converting .wasm to .wat), 
a coonverter between sequential code and folded code and a coonverter from a text file to a C file.

## New feature: generating c code for scripts (.wast files).

The convertS program can now generate C code for scripts.  Only the *module*, *invoke* and *assert_return*
commands are implemented.  The resulting C file can be compiled and run, giving error messages when an *assert_return*
command fails.

Refer to the documentation for an example.

## Installation
You require a c++17 compliant C++compiler.

Download and unpack the files.

### With scons on Linux.
In the base directory enter

     $ scons <parameters...>

The scons parameters are:

     GCC=1
        use the gcc compiler (default = clang)
     DEBUG=1
        Enable debugging in all compiles.
     NDEBUG=1
        Dissable asserts
     CACHE_DIR=<directory name>
        sets the directory where  the scons cache will be stored (default = /tmp/scons_cache_dir)

Note that scons has a *-jN* option to allow parallel compilations.


You will find the library in the 'lib' directory and the programs in the 'bin' directory.

### Without scons.
Compile all cpp files in the 'sources/lib' directory and put the resulting object files in a library
file.

Compile each of the the cpp files in 'sources/main' and link with the above library.

## Documentation

Documentation is a work in progress.
The documentation is found in 'doc/libwasm.md'.

The assembler and disassembler programs are documented.
The source for these programs ('main/webasm.cpp' and 'main/webdisasm.cpp') can be used to understand
some of the basics of the library.

The library documentation is mostly there.
It contains example programs that show the usage of the library.
