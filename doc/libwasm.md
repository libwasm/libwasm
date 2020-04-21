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

     Usage: bin/webasm [options] wat_files
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
The *-p* option specifies that the internal data structure of the assembler must be
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


#### The *-S* option.
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

     $ bin/webdisasm -h

     Usage: bin/webdisasm [options] wasm_files
     Options:
       Options:
         -a          generate source file
         -d          dump raw file content
         -h          print this help message and exit
         -o <file>   specify output file.
         -p          print formatted file content
         -P          print formatted file content with disassembled code
         -s          generate source file, using S-expressions for code
         -S          print statistics

      The '-a' option cannot be combined with either '-d', '-p' or '-P' options.
      The '-s' option implies the '-a' option.
      The '-a' or '-s' option requires an output file and allows only one wasm_file.

      If the '-o' option is given then only one input file is allowed.


#### The *-a* option.
The *-a* option specifies that a source ouput file (usually a *.wat* file) must be produced.
The name of the output file must be given with thw *-o* option.

#### The *-s* option.
The *-s* option specifies that a source ouput file (usually a *.wat* file) must be produced.
The code will be emitted in S-expression format.
The name of the output file must be given with thw *-o* option.

##### Example
     $ bin/webdisasm -s -o sample.wats sample.wasm

     $ cat sample.wats
     (module
       (type (;0;) (func (param i32 i32) (result i32)))
       (func (;0;) (type 0) (param i32 i32) (result i32) (i32.add (local.get 0) (local.get 1))))


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
     $ bin/webidisasm sample.wasm -P
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


#### The *-S* option.
The *-S* option prints assembler statistics.

<P style="page-break-before: always">

## The convertS program.

The convertS program converts a text file into a new text file.
This new text file can either be in sequential format or in S-expression format.

The start the disassembler use the command

     $ bin/convertS [*options*] -o *new_wat_file* *wat_file*

*wat_file* is the text file to convert.

*new_wat_file* is the generated file.

*options* is a list of options.

#### The *-h* option.
The *-h* option shows the help page.

##### Example

     $ bin/convertS -h

     Usage: ../bin/convertS [options] wat_file
     Options:
       -a          generate sequential assmbler file
       -s          generate S-expression assmbler file
       -h          print this help message and exit
       -o <file>   specify output file.
       -S          print statistics

     Exactly one of '-a' or '-s' must be given.

#### The *-a* option.
The *-a* option specifies that a source ouput file (usually a *.wat* file) must be produced.
The name of the output file must be given with thw *-o* option.

#### The *-s* option.
The *-s* option specifies that a source ouput file (usually a *.wat* file) must be produced.
The code will be emitted in S-expression format.
The name of the output file must be given with thw *-o* option.

##### Example

     $ bin/convertS sample.wat -s -o sample.wats
     $ cat sample.wats
     (module
       (type (;0;) (func (param i32 i32) (result i32)))
       (func (;0;) (type 0) (param $lhs i32) (param $rhs i32) (result i32)
         (i32.add (local.get 0) (local.get 1))))


## Library.

The libwasm library consists of the following major parts:

classes to assemble and disassemble programmatically:

* the *Assembler* class
* the *Disassembler* class

classes and functions to manipulate the contents:

* the backbone
* the *Context* classes
* utility functions.


### The *Assembler* class
The *Assembler* class allows to parse webassembly text and produce several outputs.

The Interface for the *Assembler* class is:

     class Assembler
     {
         public:
             Assembler(std::istream& stream)
             bool isGood() const
             void show(std::ostream& os, unsigned flags)
             void generate(std::ostream& os)
             void generateS(std::ostream& os)
             void write(std::ostream& os)
             auto getErrorCount() const;
             auto getWarningCount() const;
     }


#### *Assembler* constructor.
The constructor takes an input stream as parameter.  The webassembly text will be read from that input stream.
This allows to read from a file or a string.

The constructor will create the internal structure known as the backbone.

#### The *isGood* method.
Once the *Assembler* is instantiated, the *isGood* method returns true if the input stream was
successfuly read, false otherwise.


#### The *generate* and *generateS* methods.
The *generate* method generates webassembly text code from the backbone into the given output stream.
Code will be in sequential format.

The *generateS* method generates webassembly text code from the backbone into the given output stream.
Code will be in S_expression format.


#### The *show* method.
The *show* method generates a human-readable dump of the backbone into the given output stream.

The *flags* parameter should be set to 1 if the dump must contain disassembled code, 0 otherwise.


#### The *write* method.
The *write* method generates webassembly binary code from the backbone into the given output stream.

#### the *getErrorCount* and *getWarningCount* methods.
The *getErrorCount* and *getWarningCount* methods return the numper of errors and warnings
that were given during preceding processing.

##### Example
The following program reads and parses the test file 'sample.wat', produces a human readable dump of
the backbone and generates the binary file 'sample.wasm'.

     #include "Assembler.h"

     #include <fstream>
     #include <iostream>

     int main()
     {
         std::ifstream inputStream("sample.wat", std::ios::binary);if (!inputStream.good()) {
             std::cerr << "Error: Unable to open input file 'sample.wat'\n";
             exit(1);
         }

         Assembler assembler(inputStream);

         if (assembler.isGood()) {
             assembler.show(std::cout, 0);

             std::ofstream outputStream("sample.wasm");

             if (!outputStream.good()) {
                 std::cerr << "Error: Unable to open output file 'sample.wasm'\n";
                 exit(1);
             }

             assembler.write(outputStream);
         }
     }

This program can be compiled with the following command:

     $ clang++ -o test -std=c++17 test.cpp -I libwasmdir/sources/lib -L libwasmdir/lib -lwasm

where 'test.cpp' is de name of the source file and 'libwasmdir' is de directory where libwasm is installed.

'g++' can be substituted for 'clang++'.

### The Disassembler class

The *Disassembler* class allows to read webassembly binary input and produce several outputs.

The Interface for the *Assembler* class is:

class Disassembler
{
    public:
        Disassembler(std::istream& stream)'
        bool isGood() const;
        void dump(std::ostream& os);
        void show(std::ostream& os, unsigned flags);
        void generate(std::ostream& os);
        void write(std::ostream& os);
        auto getErrorCount() const;
        auto getWarningCount() const;
}

#### *Disassembler* constructor.
The constructor takes an input stream as parameter.  The webassembly binary will be read from that input stream.
This allows to read from a file or a string.

The constructor will create the internal structure known as the backbone.

#### Other methods.
Other methods have the same functionality as for the *Assembler* class.

<P style="page-break-before: always">

### The backbone
The backbone is a set of classes that describe all the elements from the webassembly syntax and its
binary representation.
The instances of these classes will produce a syntak tree with a Module instance as root.

The "Module.h" and Module.cpp" describe the Module class.

The *BackBone.h* and *BackBone.cpp* files describe all other  classes in the backbone except the instructions.

The instructions are described in the *Instruction.h* and the *Instruction.cpp* files.

#### The Module class.
The module class is the root of the syntax tree for a webassembly module.

The module class contains pointers to all the sections.  It also maintains a lot of other information,
such as type count, function count etc.  This information is built up gradually during parsing a text file or
reading a binary file.

Methods are provided to manipulate all that data.

#### The section classes.
There is a class for each section type. The section types are decribed by the class
SectionType iin *Encodings.h*.

All Section classes have a very similar API.

##### Example

     class TypeSection : public Section
     {
         public:

             ...

             virtual void show(std::ostream& os, Context& context, unsigned flags = 0) override;
             virtual void generate(std::ostream& os, Context& context, unsigned flags = 0) override;
             virtual void write(BinaryContext& context) const override;
             virtual void check(CheckContext& context) override;

             static void read(BinaryContext& context, TypeSection* result);

         private:
             std::vector<std::unique_ptr<TypeDeclaration>> types;
     };

All section types implement the function *show*, *generate*, *write* and *read*.

The *read* method reads the from a binary input.

The *write* method writes the section to binary output.

The *generate* method generates text format for the section to a source file.

The *show* method generates readable output of the section. (see the *-p* option of *webasm* or *webdisasm*).

The *check* method performs consistency checks on the section.


Most sections contain a vector of entries (*types* in the example).  All these entries have a similar structure.

##### Example
class TypeDeclaration
{
    public:

        ...

        void show(std::ostream& os, Context& context);
        void generate(std::ostream& os, Context& context);
        void check(CheckContext& context);
        void write(BinaryContext& context) const;

        static TypeDeclaration* parse(SourceContext& context);
        static TypeDeclaration* read(BinaryContext& context);

    private:
        std::unique_ptr<Signature> signature;
        uint32_t number = 0;
        std::string id;
};

The *read*, *write*, *generate*, *check* and *show* functions have the same purpose as described for the sections.

The *parse* function parses an entry from a source input file.


The section entry classes will have in turn pointers to the elements they create and so on.  All these elements
will have a similar API.


#### Instructions.
The webassembly instructions are divided according the type of immediate they take.  For every one of these types
there is a separate class.  All these classes implement a similar API as described above.

##### Example
     class InstructionTable : public Instruction
     {
         public:

             ...

             virtual void write(BinaryContext& context) override;
             virtual void generate(std::ostream& os, InstructionContext& context) override;
             virtual void check(CheckContext& context) override;

             static InstructionTable* parse(SourceContext& context, Opcode opcode);
             static InstructionTable* read(BinaryContext& context);

         protected:
             std::vector<uint32_t> labels;
             uint32_t defaultLabel = 0;
     };

The immediate types can be found in the file "Encodings.h".

The instruction parsing process is table driven.  This table (Opcode::info) can be found in the file Encodings.cpp.

Each entry in the table is a struct describing one instruction:

     struct Info
     {
         uint32_t opcode;
         ImmediateType type = ImmediateType::none;
         SignatureCode signatureCode = SignatureCode::void_;
         std::string_view name;
         uint32_t align = 0;
     };

The *opcode* field contains the prefix and code if the instruction as described in the webassembly
documentation.  The opcode is built as ((prefix << 24) | code).

The *type* field contains the immediate type.

The *signatureCode* field contains a code thet descripes the parameters and result of the instruction.
This code is an entry of the enum *SignatureCode", found in "Encodings.h".  The code consists of some
value types separated by underscores.  The result type (or *void*) comes first, separated with 2 underscores
from the parameter types which are separated by one underscore.

##### Examples
     i32__i64_i64    ;; The instruction expects two i64 values on the stack
                     ;; and returns an i32 value on the stack
     void_           ;; The instruction does not access the stack
     f64_            ;; The instruction returns an f64 value on the stack

The *name* field contains the name of the instruction as it appears in a text file.

The *align* file specifies the memory alignment for the instruction.

### The Context classes

To be done.


