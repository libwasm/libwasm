# libwasm

Linwasm is a library to manipulate webassembly files from C++.
It also contains an assembler (converting .wat to .wasm) and a dissassembler (converting .wasm to .wat).

to install:
- You require the 'scons' package as well as a c++17 compliant C++compiler.
- download and unpack the files.
- In the base directory enter
   $ scons <parameters...>
   You will find the library in the 'lib' directory and the programs in the 'bin' directory.

The scons parameters are:
   GCC=1
      use the gcc compiler (default = clang)
   DEBUG=1
      Enable debugging in all compiles.
   NDEBUG=1
      Dissable asserts
   CACHE_DIR=<directory name>
      sets the directory where  the scons cache will be stored (default = /tmp/scons_cache_dir)
      
Documentation is a work in progress and will be added soon.
