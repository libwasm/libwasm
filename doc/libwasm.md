# libwasm

Linwasm is a library to manipulate webassembly files from C++.
It also contains an assembler (converting .wat to .wasm) and a dissassembler (converting .wasm to .wat)

The assembler and disassembler are described first, because they can be used out of the box and require
little documentation.

The libray will be described last.

<P style="page-break-before: always">

## Assembler.

The start the assembler use the command

     $ bin/webasm [*options*] *wat_files*

*wat_files* is a list of one or more webassembly sources files, usually with the '.wat' extension.

*options* is a list of options.

### Options
In the description of the options, the following example source file (called 'sample.wat') will be used:

      (module
        (type (;0;) (func (param i32 i32) (result i32)))

        (func (param $lhs i32) (param $rhs i32) (result i32)
          local.get $lhs
          local.get $rhs
          i32.add)
      )

#### The *-h* option.
The *-h* option shows the help page.

##### Example
     $ bin/webasm -h

     Usage: /home/jvd/projects/libwasm/bin/webasm [options] wat_files
     Options:
       -a          generate binary file
       -h          print this help message and exit
       -o <file>   specify output file.
       -p          print formatted file content
       -P          print formatted file content with dassembled code
       -S          print statistics

     The '-a' option cannot be combined with either ''-p' or '-P' options.
     The '-a' option requires an output file and allows only one wat_file.

     If the '-o' option is given then only one input file is allowed.

     For the '-p' and '-P' options the output file defaults to 'std::cout'.

#### The *-a* option.
The *-a* option specifies that a binary ouput file (usually a *.wasm* file) must be produced.
The name of the output file must be given with thw *-o* option.

#### The *-o* option.
The *-o* option specifies the output file to be written.

If the *-a* option is also given then the output file will be a binary (usually *.wasm* file).

If any of the the *-p* or *-P* options is also given then the output file
will be a text file.

#### The *-p* option.
The *-p* option specifies that the internal data structure of the assembelr must be
dumped in a human readable format.  The internal data structure represents all the sections and
their content parsed from the inout file.

##### Example
     $ bin/webasm sample.wat -p

     Type section:
       Type 0:  (i32, i32) -> i32

     Function section:
       func 0:  signature index="0"  ($lhs i32, $rhs i32) -> i32

     Code section:
       Code for function 0

     DataCount section:
       data count: 0

#### The *-P* option.
The *-P* produces the same output as the *-p* option, but adds the code to each function.

##### Example
     $bin/webasm sample.wat -P
     Type section:
       Type 0:  (i32, i32) -> i32

     Function section:
       func 0:  signature index="0"  ($lhs i32, $rhs i32) -> i32

     Code section:
       Code for function 0:
         local.get 0
         local.get 1
         i32.add

     DataCount section:
       data count: 0

#### The *-s* option.
The *-S* option prints assembler statistics.

## Disassembler.

The start the disassembler use the command

     $ bin/webdisasm [*options*] *wasm_files*

*wasm_files* is a list of one or more webassembly binary files, usually with the '.wasm' extension.

*options* is a list of options.

### Options
In the description of the options, The binary file (called 'sample.wasm') will be used that
correspond to the 'sample.wat' file in the description of the assembler.

#### The *-h* option.
The *-h* option shows the help page.

##### Example

     webdisasm -h

     Usage: /home/jvd/projects/libwasm/bin/webdisasm [options] wasm_files
     Options:
       -a          generate source file
       -d          dump raw file content
       -h          print this help message and exit
       -o <file>   specify output file.
       -p          print formatted file content
       -P          print formatted file content with disassembled code
       -S          print statistics

     The '-a' option cannot be combined with either '-d', '-p' or '-P' options.
     The '-a' option requires an output file and allows only one wasm_file.

     If the '-o' option is given then only one input file is allowed.

     For the '-d', '-p' and '-P' options the output file defaults to 'std::cout'.

#### The *-a* option.
The *-a* option specifies that a source ouput file (usually a *.wat* file) must be produced.
The name of the output file must be given with thw *-o* option.

#### The *-o* option.
The *-o* option specifies the output file to be written.

If the *-a* option is also given then the output file will be a binary (usually *.wtm* file).

If any of the the *-d*, *-p* or *-P* options is also given then the output file
will be a text file.

#### The *-p* option.
The *-p* option specifies that the internal data structure of the assembelr must be
dumped in a human readable format.  The internal data structure represents all the sections and
their content parsed from the inout file.

##### Example
     $ bin/webdisasm sample.wasm -p

     Type section:
       Type 0:  (i32, i32) -> i32

     Function section:
       func 0:  signature index="0"  ($lhs i32, $rhs i32) -> i32

     Code section:
       Code for function 0

     DataCount section:
       data count: 0

#### The *-P* option.
The *-P* produces the same output as the *-p* option, but adds the code to each function.

##### Example
     $bin/webidisasm sample.wasm -P
     Type section:
       Type 0:  (i32, i32) -> i32

     Function section:
       func 0:  signature index="0"  ($lhs i32, $rhs i32) -> i32

     Code section:
       Code for function 0:
         local.get 0
         local.get 1
         i32.add

     DataCount section:
       data count: 0

#### The *-d* option.
the *-d* option specifies that the raw content of all sections must be dumped.

##### Example
     webdisasm sample.wasm -d

     Type section:
     0000000a:  01 60 02 7f 7f 01 7f                                .`.....

     Function section:
     00000013:  01 00                                               ..

     DataCount section:
     00000017:  00                                                  .

     Code section:
     0000001a:  01 07 00 20 00 20 01 6a  0b                         ... . .j.

The *-d* option can be combined with the *-p* or *-P* option

#### The *-s* option.
The *-S* option prints assembler statistics.

