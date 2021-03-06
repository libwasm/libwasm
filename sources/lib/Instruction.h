// Instruction.h

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "BackBone.h"
#include "Context.h"
#include "DataBuffer.h"
#include "Encodings.h"
#include "TreeNode.h"

#include <array>
#include <iostream>
#include <memory>
#include <vector>

namespace libwasm
{

class CodeEntry;

class InstructionContext
{
    public:
        InstructionContext(Module* module, CodeEntry* code = nullptr)
          : module(module), code(code)
        {
        }

        unsigned enterBlock();
        void leaveBlock();

        void enter();
        void leave();

        unsigned getBlockDepth() const
        {
            return blockDepth;
        }

        std::string_view getIndent() {
            return indent;
        }

        void setComments(bool v)
        {
            comments = v;
        }

        auto getComments() const
        {
            return comments;
        }

        auto* getModule()
        {
            return module;
        }

        auto* getCode()
        {
            return code;
        }

    private:
        unsigned blockDepth = 0;
        bool comments = true;
        Module* module = nullptr;
        CodeEntry* code = nullptr;
        std::string indent;
};

class Instruction : public TreeNode
{
    public:
        Instruction() = default;

        Instruction(ImmediateType p)
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

        auto getImmediateType() const
        {
            return opcode.getImmediateType();
        }

        void writeOpcode(BinaryContext& context) const;

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
        static Opcode readOpcode(BinaryContext& context);

    protected:
        Opcode opcode;
        ImmediateType encoding;
};

class InstructionNone : public Instruction
{
    public:
        InstructionNone()
          : Instruction(ImmediateType::none)
        {
        }

        InstructionNone(Opcode op)
          : Instruction(ImmediateType::none)
        {
            opcode = op;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionNone* parse(SourceContext& context, Opcode opcode);
        static InstructionNone* read(BinaryContext& context);
};

class InstructionSelect : public Instruction
{
    public:
        InstructionSelect()
          : Instruction(ImmediateType::select)
        {
        }

        auto getType() const
        {
            return type;
        }

        void setType(ValueType t)
        {
            type = t;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionSelect* parse(SourceContext& context, Opcode opcode);
        static InstructionSelect* read(BinaryContext& context, Opcode opcode);

    private:
        ValueType type = ValueType::void_;
};

class InstructionI32 : public Instruction
{
    public:
        InstructionI32()
          : Instruction(ImmediateType::i32)
        {
        }

        auto getValue() const
        {
            return value;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionI32* parse(SourceContext& context, Opcode opcode);
        static InstructionI32* read(BinaryContext& context);

    protected:
        int32_t value = 0;
};

class InstructionI64 : public Instruction
{
    public:
        InstructionI64()
          : Instruction(ImmediateType::i64)
        {
        }

        auto getValue() const
        {
            return value;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionI64* parse(SourceContext& context, Opcode opcode);
        static InstructionI64* read(BinaryContext& context);

    protected:
        int64_t value = 0;
};

class InstructionF32 : public Instruction
{
    public:
        InstructionF32()
          : Instruction(ImmediateType::f32)
        {
        }

        auto getValue() const
        {
            return value;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        void generateCValue(std::ostream& os, InstructionContext& context);
        virtual void check(CheckContext& context) override;

        static InstructionF32* parse(SourceContext& context, Opcode opcode);
        static InstructionF32* read(BinaryContext& context);

    protected:
        float value = 0;
};

class InstructionF64 : public Instruction
{
    public:
        InstructionF64()
          : Instruction(ImmediateType::f64)
        {
        }

        auto getValue() const
        {
            return value;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        void generateCValue(std::ostream& os, InstructionContext& context);
        virtual void check(CheckContext& context) override;

        static InstructionF64* parse(SourceContext& context, Opcode opcode);
        static InstructionF64* read(BinaryContext& context);

    protected:
        double value = 0;
};

class InstructionV128 : public Instruction
{
    public:
        InstructionV128()
          : Instruction(ImmediateType::f64)
        {
        }

        v128_t getValue() const
        {
            return value;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionV128* parse(SourceContext& context, Opcode opcode);
        static InstructionV128* read(BinaryContext& context);

    protected:
        v128_t value = { 0 };
};

class InstructionBlock : public TypeUse, public Instruction
{
    public:
        InstructionBlock()
          : Instruction(ImmediateType::block)
        {
        }

        virtual ~InstructionBlock() override
        {
        }

        auto getResultType() const
        {
            return resultType;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionBlock* parse(SourceContext& context, Opcode opcode);
        static InstructionBlock* read(BinaryContext& context);

    protected:
        ValueType resultType = ValueType::void_;
        std::string label;
};

class InstructionIdx : public Instruction
{
    public:
        InstructionIdx()
          : Instruction(ImmediateType::idx)
        {
        }

        void setIndex(uint32_t i)
        {
            index = i;
        }

        auto getIndex() const
        {
            return index;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;

        static InstructionIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionIdx* read(BinaryContext& context);

    protected:
        InstructionIdx(ImmediateType encoding)
             : Instruction(encoding)
        {
        }

        uint32_t index = 0;
};

class InstructionLocalIdx : public InstructionIdx
{
    public:
        InstructionLocalIdx()
          : InstructionIdx(ImmediateType::localIdx)
        {
        }

        static InstructionLocalIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionLocalIdx* read(BinaryContext& context);

};

class InstructionFunctionIdx : public InstructionIdx
{
    public:
        InstructionFunctionIdx()
          : InstructionIdx(ImmediateType::functionIdx)
        {
        }

        virtual void generate(std::ostream& os, InstructionContext& context) override;

        static InstructionFunctionIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionFunctionIdx* read(BinaryContext& context);
};

class InstructionGlobalIdx : public InstructionIdx
{
    public:
        InstructionGlobalIdx()
          : InstructionIdx(ImmediateType::globalIdx)
        {
        }

        virtual void generate(std::ostream& os, InstructionContext& context) override;

        static InstructionGlobalIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionGlobalIdx* read(BinaryContext& context);
};

class InstructionLabelIdx : public InstructionIdx
{
    public:
        InstructionLabelIdx()
          : InstructionIdx(ImmediateType::labelIdx)
        {
        }

        static InstructionLabelIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionLabelIdx* read(BinaryContext& context);
};

class InstructionEventIdx : public InstructionIdx
{
    public:
        InstructionEventIdx()
          : InstructionIdx(ImmediateType::eventIdx)
        {
        }

        virtual void generate(std::ostream& os, InstructionContext& context) override;

        static InstructionEventIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionEventIdx* read(BinaryContext& context);
};

class InstructionSegmentIdx : public InstructionIdx
{
    public:
        InstructionSegmentIdx()
          : InstructionIdx(ImmediateType::segmentIdx)
        {
        }

        static InstructionSegmentIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionSegmentIdx* read(BinaryContext& context);
};

class InstructionElementIdx : public InstructionIdx
{
    public:
        InstructionElementIdx()
          : InstructionIdx(ImmediateType::elementIdx)
        {
        }

        static InstructionElementIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionElementIdx* read(BinaryContext& context);
};

class InstructionLane2Idx : public InstructionIdx
{
    public:
        InstructionLane2Idx()
          : InstructionIdx(ImmediateType::lane2Idx)
        {
        }

        static InstructionLane2Idx* parse(SourceContext& context, Opcode opcode);
        static InstructionLane2Idx* read(BinaryContext& context);
};

class InstructionLane4Idx : public InstructionIdx
{
    public:
        InstructionLane4Idx()
          : InstructionIdx(ImmediateType::lane4Idx)
        {
        }

        static InstructionLane4Idx* parse(SourceContext& context, Opcode opcode);
        static InstructionLane4Idx* read(BinaryContext& context);
};

class InstructionLane8Idx : public InstructionIdx
{
    public:
        InstructionLane8Idx()
          : InstructionIdx(ImmediateType::lane8Idx)
        {
        }

        static InstructionLane8Idx* parse(SourceContext& context, Opcode opcode);
        static InstructionLane8Idx* read(BinaryContext& context);
};

class InstructionLane16Idx : public InstructionIdx
{
    public:
        InstructionLane16Idx()
          : InstructionIdx(ImmediateType::lane16Idx)
        {
        }

        static InstructionLane16Idx* parse(SourceContext& context, Opcode opcode);
        static InstructionLane16Idx* read(BinaryContext& context);
};

class InstructionLane32Idx : public InstructionIdx
{
    public:
        InstructionLane32Idx()
          : InstructionIdx(ImmediateType::lane32Idx)
        {
        }

        static InstructionLane32Idx* parse(SourceContext& context, Opcode opcode);
        static InstructionLane32Idx* read(BinaryContext& context);
};

class InstructionShuffle : public Instruction
{
    public:
        InstructionShuffle()
          : Instruction(ImmediateType::shuffle)
        {
        }

        auto& getValue() const
        {
            return value;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionShuffle* parse(SourceContext& context, Opcode opcode);
        static InstructionShuffle* read(BinaryContext& context);

    protected:
        v128_t value;
};

class InstructionBrTable : public Instruction
{
    public:
        InstructionBrTable()
          : Instruction(ImmediateType::brTable)
        {
        }

        void addLabel(uint32_t i)
        {
            labels.push_back(i);
        }

        auto getDefaultLabel() const
        {
            return defaultLabel;
        }

        auto& getLabels()
        {
            return labels;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionBrTable* parse(SourceContext& context, Opcode opcode);
        static InstructionBrTable* read(BinaryContext& context);

    protected:
        std::vector<uint32_t> labels;
        uint32_t defaultLabel = 0;
};

class InstructionMemory : public Instruction
{
    public:
        InstructionMemory()
          : Instruction(ImmediateType::memory)
        {
        }

        auto getOffset() const
        {
            return offset;
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

class InstructionIndirect : public Instruction
{
    public:
        InstructionIndirect()
          : Instruction(ImmediateType::indirect)
        {
        }

        auto getTypeIndex() const
        {
            return typeIndex;
        }

        auto getTableIndex() const
        {
            return tableIndex;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionIndirect* parse(SourceContext& context, Opcode opcode);
        static InstructionIndirect* read(BinaryContext& context);

    protected:
        uint32_t typeIndex = 0;
        uint8_t tableIndex = 0;
};

class InstructionDepthEventIdx : public Instruction
{
    public:
        InstructionDepthEventIdx()
          : Instruction(ImmediateType::depthEventIdx)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionDepthEventIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionDepthEventIdx* read(BinaryContext& context);

    protected:
        uint32_t depth = 0;
        uint32_t eventIndex = 0;
};

class InstructionSegmentIdxMem : public Instruction
{
    public:
        InstructionSegmentIdxMem()
          : Instruction(ImmediateType::segmentIdxMem)
        {
        }

        auto getSegmentIndex() const
        {
            return segmentIndex;
        }

        auto getMemory() const
        {
            return memory;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionSegmentIdxMem* parse(SourceContext& context, Opcode opcode);
        static InstructionSegmentIdxMem* read(BinaryContext& context);

    protected:
        uint32_t segmentIndex = 0;
        uint8_t memory = 0;
};

class InstructionMem0 : public Instruction
{
    public:
        InstructionMem0()
          : Instruction(ImmediateType::mem0)
        {
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionMem0* parse(SourceContext& context, Opcode opcode);
        static InstructionMem0* read(BinaryContext& context);
};

class InstructionMem : public Instruction
{
    public:
        InstructionMem()
            : Instruction(ImmediateType::mem)
        {
        }

        auto getMemory() const
        {
            return memory;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionMem* parse(SourceContext& context, Opcode opcode);
        static InstructionMem* read(BinaryContext& context);

    protected:
        uint8_t memory = 0;
};

class InstructionMemMem : public Instruction
{
    public:
        InstructionMemMem()
          : Instruction(ImmediateType::memMem)
        {
        }

        auto getDestination() const
        {
            return destination;
        }

        auto getSource() const
        {
            return source;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionMemMem* parse(SourceContext& context, Opcode opcode);
        static InstructionMemMem* read(BinaryContext& context);

    protected:
        uint8_t destination = 0;
        uint8_t source = 0;
};

class InstructionTableElementIdx : public Instruction
{
    public:
        InstructionTableElementIdx()
          : Instruction(ImmediateType::tableElementIdx)
        {
        }

        auto getElementIndex() const
        {
            return elementIndex;
        }

        auto getTableIndex() const
        {
            return tableIndex;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionTableElementIdx* parse(SourceContext& context, Opcode opcode);
        static InstructionTableElementIdx* read(BinaryContext& context);

    protected:
        uint32_t elementIndex = 0;
        uint8_t tableIndex = 0;
};

class InstructionTable : public Instruction
{
    public:
        InstructionTable()
            : Instruction(ImmediateType::table)
        {
        }

        auto getIndex() const
        {
            return tableIndex;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionTable* parse(SourceContext& context, Opcode opcode);
        static InstructionTable* read(BinaryContext& context);

    protected:
        uint8_t tableIndex = 0;
};

class InstructionTableTable : public Instruction
{
    public:
        InstructionTableTable()
          : Instruction(ImmediateType::tableTable)
        {
        }

        auto getDestination() const
        {
            return destination;
        }

        auto getSource() const
        {
            return source;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionTableTable* parse(SourceContext& context, Opcode opcode);
        static InstructionTableTable* read(BinaryContext& context);

    protected:
        uint8_t destination = 0;
        uint8_t source = 0;
};

class InstructionRefType : public Instruction
{
    public:
        InstructionRefType()
          : Instruction(ImmediateType::i32)
        {
        }

        auto getType() const
        {
            return type;
        }

        virtual void write(BinaryContext& context) override;
        virtual void generate(std::ostream& os, InstructionContext& context) override;
        virtual void check(CheckContext& context) override;

        static InstructionRefType* parse(SourceContext& context, Opcode opcode);
        static InstructionRefType* read(BinaryContext& context);

    protected:
        ValueType type = ValueType::void_;
};

};

#endif

