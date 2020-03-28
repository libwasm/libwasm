// Instruction.h

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "DataBuffer.h"
#include "Context.h"
#include "Encodings.h"
#include "TreeNode.h"

#include <iostream>
#include <memory>
#include <vector>

class InstructionContext
{
    public:
        unsigned enterBlock()
        {
            return ++blockDepth;
        }

        void leaveBlock()
        {
            if (blockDepth > 0) {
                --blockDepth;
            }
        }

        unsigned getBlockDepth() const
        {
            return blockDepth;
        }

    private:
        unsigned blockDepth = 0;
};

class Instruction : public TreeNode
{
    public:
        Instruction() = default;

        Instruction(ParameterEncoding p)
          : encoding(p)
        {
        }

        void setOpcode(uint8_t c)
        {
            opcode = Opcode(c);
        }

        void setOpcode(Opcode c)
        {
            opcode = c;
        }

        Opcode getOpcode() const
        {
            return opcode;
        }

        auto getParameterEncoding() const
        {
            return opcode.getParameterEncoding();
        }

        virtual void write(BinaryContext& context)
        {
        }

        virtual void generate(std::ostream& os, InstructionContext& context)
        {
        }

        virtual void check(CheckContext& context)
        {
        }

        static Instruction* parse(SourceContext& context);
        static void parse(SourceContext& context, std::vector<Instruction*>& dest);
        static bool parseFolded(SourceContext& context, std::vector<Instruction*>& dest);
        static Instruction* read(BinaryContext& context);

    protected:
        Opcode opcode;
        ParameterEncoding encoding;
};

class InstructionNone : public Instruction
{
    public:
        InstructionNone()
          : Instruction(ParameterEncoding::none)
        {
        }

        InstructionNone(Opcode op)
          : Instruction(ParameterEncoding::none)
        {
            opcode = op;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionNone* parse(SourceContext& context, Opcode opcode);
        static InstructionNone* read(BinaryContext& context);
};

class InstructionI32 : public Instruction
{
    public:
        InstructionI32()
          : Instruction(ParameterEncoding::i32)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionI32* parse(SourceContext& context, Opcode opcode);
        static InstructionI32* read(BinaryContext& context);

    protected:
        int32_t imm = 0;
};

class InstructionI64 : public Instruction
{
    public:
        InstructionI64()
          : Instruction(ParameterEncoding::i64)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionI64* parse(SourceContext& context, Opcode opcode);
        static InstructionI64* read(BinaryContext& context);

    protected:
        int64_t imm = 0;
};

class InstructionF32 : public Instruction
{
    public:
        InstructionF32()
          : Instruction(ParameterEncoding::f32)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionF32* parse(SourceContext& context, Opcode opcode);
        static InstructionF32* read(BinaryContext& context);

    protected:
        float imm = 0;
};

class InstructionF64 : public Instruction
{
    public:
        InstructionF64()
          : Instruction(ParameterEncoding::f64)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionF64* parse(SourceContext& context, Opcode opcode);
        static InstructionF64* read(BinaryContext& context);

    protected:
        double imm = 0;
};

class InstructionBlock : public Instruction
{
    public:
        InstructionBlock()
          : Instruction(ParameterEncoding::block)
        {
        }

        virtual ~InstructionBlock() override
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionBlock* parse(SourceContext& context, Opcode opcode);
        static InstructionBlock* read(BinaryContext& context);

    protected:
        ValueType imm = ValueType::void_;
        std::string label;
};

class InstructionIdx : public Instruction
{
    public:
        InstructionIdx()
          : Instruction(ParameterEncoding::idx)
        {
        }

        void setIndex(uint32_t index)
        {
            imm = index;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;

        static InstructionIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionIdx* read(BinaryContext& context);

    protected:
        InstructionIdx(ParameterEncoding encoding)
             : Instruction(encoding)
        {
        }

        uint32_t imm = 0;
};

class InstructionLocalIdx : public InstructionIdx
{
    public:
        InstructionLocalIdx()
          : InstructionIdx(ParameterEncoding::localIdx)
        {
        }

        static InstructionLocalIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionLocalIdx* read(BinaryContext& context);
};

class InstructionFunctionIdx : public InstructionIdx
{
    public:
        InstructionFunctionIdx()
          : InstructionIdx(ParameterEncoding::functionIdx)
        {
        }

        static InstructionFunctionIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionFunctionIdx* read(BinaryContext& context);
};

class InstructionGlobalIdx : public InstructionIdx
{
    public:
        InstructionGlobalIdx()
          : InstructionIdx(ParameterEncoding::globalIdx)
        {
        }

        static InstructionGlobalIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionGlobalIdx* read(BinaryContext& context);
};

class InstructionLabelIdx : public InstructionIdx
{
    public:
        InstructionLabelIdx()
          : InstructionIdx(ParameterEncoding::labelIdx)
        {
        }

        static InstructionLabelIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionLabelIdx* read(BinaryContext& context);
};

class InstructionTable : public Instruction
{
    public:
        InstructionTable()
          : Instruction(ParameterEncoding::table)
        {
        }

        void addLabel(uint32_t i)
        {
            labels.push_back(i);
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionTable* parse(SourceContext& context, Opcode opcode);
        static InstructionTable* read(BinaryContext& context);

    protected:
        std::vector<uint32_t> labels;
        uint32_t defaultLabel = 0;
};

class InstructionMemory : public Instruction
{
    public:
        InstructionMemory()
          : Instruction(ParameterEncoding::memory)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionMemory* parse(SourceContext& context, Opcode opcode);
        static InstructionMemory* read(BinaryContext& context);

    protected:
        uint32_t offset = 0;
        uint32_t alignPower = 0;
};

class InstructionMemory0 : public Instruction
{
    public:
        InstructionMemory0()
          : Instruction(ParameterEncoding::memory0)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionMemory0* parse(SourceContext& context, Opcode opcode);
        static InstructionMemory0* read(BinaryContext& context);
};

class InstructionIndirect : public Instruction
{
    public:
        InstructionIndirect()
          : Instruction(ParameterEncoding::idx)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionIndirect* parse(SourceContext& context, Opcode opcode);
        static InstructionIndirect* read(BinaryContext& context);

    protected:
        uint32_t typeIndex = 0;
        uint32_t dummy = 0;
};

#endif

