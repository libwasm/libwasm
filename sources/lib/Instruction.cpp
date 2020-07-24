// Instruction.cpp

#include "Instruction.h"

#include "BackBone.h"
#include "Module.h"
#include "TokenBuffer.h"
#include "parser.h"

#include <cassert>
#include <iomanip>

namespace libwasm
{

unsigned InstructionContext::enterBlock()
{
    indent.append("  ");
    return ++blockDepth;
}

void InstructionContext::leaveBlock()
{
    if (blockDepth > 0) {
        --blockDepth;
        if (indent.size() > 1) {
            indent.resize(indent.size() - 2);
        }
    }
}

void InstructionContext::enter()
{
    indent.append("  ");
}

void InstructionContext::leave()
{
    if (indent.size() > 1) {
        if (indent.size() > 1) {
            indent.resize(indent.size() - 2);
        }
    }
}

void Instruction::writeOpcode(BinaryContext& context) const
{
    auto& data = context.data();
    auto prefix = OpcodePrefix(opcode.getPrefix());
    auto code = opcode.getCode();

    if (prefix.isValid()) {
        data.putU8(uint8_t(prefix));
        data.putU32leb(code);
    } else {
        data.putU8(uint8_t(code));
    }
}

InstructionNone* InstructionNone::parse(SourceContext& context, Opcode opcode)
{
    auto* module = context.getModule();
    auto& tokens = context.tokens();

    if (opcode == Opcode::end || opcode == Opcode::else_) {
        if (auto id = context.getId()) {
            auto index = module->getLabelIndex(*id);

            context.msgs().errorWhen(index != 0, tokens.peekToken(-1),
                    "Id must be equal to the label of the innermost block.");
        }
    }

    if (opcode == Opcode::end) {
        module->popLabel();
    }

    return context.makeTreeNode<InstructionNone>();
}

InstructionNone* InstructionNone::read(BinaryContext& context)
{
    return context.makeTreeNode<InstructionNone>();
}

void InstructionNone::write(BinaryContext& context)
{
    writeOpcode(context);
}

void InstructionNone::check(CheckContext& context)
{
    // nothing to do
}

void InstructionNone::generate(std::ostream& os, InstructionContext& context)
{
    if (opcode == Opcode::end) {
        context.leaveBlock();
    }

    os << opcode;
}

void InstructionSelect::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    if (opcode == Opcode::selectV) {
        data.putI32leb(int32_t(type));
    }
}

void InstructionSelect::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;

    if (opcode == Opcode::selectV) {
        os << ' ' << type;
    }
}

void InstructionSelect::check(CheckContext& context)
{
    if (opcode == Opcode::selectV) {
        context.checkValueType(this, type);
    }
}


InstructionSelect* InstructionSelect::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionSelect>();

    if (startClause(context, "result")) {
        auto& tokens = context.tokens();

        result->type = requiredValueType(context);
        result->opcode = Opcode::selectV;

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
        }
    } else {
        result->opcode = Opcode::select;
    }

    return result;
}

InstructionSelect* InstructionSelect::read(BinaryContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionSelect>();

    if (opcode == Opcode::selectV) {
        result->type = context.data().getI32leb();

        context.msgs().errorWhen(!result->type.isValid(),
                "Invalid value type ", int32_t(result->type));
    }

    return result;
}

InstructionI32* InstructionI32::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionI32>();

    result->value = requiredI32(context);

    return result;
}

InstructionI32* InstructionI32::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionI32>();

    result->value = context.data().getI32leb();

    return result;
}

void InstructionI32::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putI32leb(value);
}

void InstructionI32::check(CheckContext& context)
{
    // nothing to do
}

void InstructionI32::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << value;
}

InstructionI64* InstructionI64::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionI64>();

    result->value = requiredI64(context);

    return result;
}

InstructionI64* InstructionI64::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionI64>();

    result->value = context.data().getI64leb();

    return result;
}

void InstructionI64::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putI64leb(value);
}

void InstructionI64::check(CheckContext& context)
{
    // nothing to do
}

void InstructionI64::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << value;
}

InstructionF32* InstructionF32::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionF32>();

    result->value = requiredF32(context);

    return result;
}

InstructionF32* InstructionF32::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionF32>();

    result->value = context.data().getF();

    return result;
}

void InstructionF32::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putF(value);
}

void InstructionF32::check(CheckContext& context)
{
    // nothing to do
}

void InstructionF32::generate(std::ostream& os, InstructionContext& context)
{
    auto flags = os.flags();

    union
    {
        uint32_t i;
        float f;
    };

    os << opcode << ' ';

    f = value;

    if ((i & 0x7f800000) == 0x7f800000) {
        if ((i & 0x80000000) != 0) {
            os << '-';
            i &= 0x7fffffff;
        }

        if (i  == 0x7f800000) {
            os << "inf";
        } else {
            os << "nan";
            if (i != 0x7fc00000) {
                os << ":0x" << std::hex << (i & 0x7fffff) << std::dec;
            }
        }
    } else  {
        os << std::hexfloat << value << std::defaultfloat;

        if (context.getComments()) {
            os << " (;=" << value << ";)";
        }
    }

    os.flags(flags);
}

void InstructionF32::generateCValue(std::ostream& os, InstructionContext& context)
{
    union
    {
        uint32_t i;
        float f;
    };

    f = value;

    if ((i & 0x7f800000) == 0x7f800000) {
        if ((i & 0x80000000) != 0) {
            os << '-';
            i &= 0x7fffffff;
        }

        if (i  == 0x7f800000) {
            os << "INFINITY";
        } else {
            if (i != 0x7fc00000) {
                os << "nan(";
                os << "\"0x" << std::hex << (i & 0x7fffff) << std::dec << "\")";
            } else {
                os << "NAN";
            }
        }
    } else  {
        os << value;
    }
}

InstructionF64* InstructionF64::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionF64>();

    result->value = requiredF64(context);

    return result;
}

InstructionF64* InstructionF64::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionF64>();

    result->value = context.data().getD();

    return result;
}

void InstructionF64::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putD(value);
}

void InstructionF64::check(CheckContext& context)
{
    // nothing to do
}

void InstructionF64::generate(std::ostream& os, InstructionContext& context)
{
    auto flags = os.flags();

    union
    {
        uint64_t i;
        double f;
    };

    os << opcode << ' ';

    f = value;

    if ((i & 0x7ff0000000000000ull) == 0x7ff0000000000000ull) {
        if ((i & 0x8000000000000000) != 0) {
            os << '-';
            i &= 0x7fffffffffffffff;
        }

        if (i  == 0x7ff0000000000000ull) {
            os << "inf";
        } else {
            os << "nan";
            if (i != 0x7ff8000000000000ull) {
                os << ":0x" << std::hex << (i & 0xfffffffffffffull) << std::dec;
            }
        }
    } else  {
        os << std::hexfloat << value << std::defaultfloat;

        if (context.getComments()) {
            os << " (;=" << value << ";)";
        }
    }

    os.flags(flags);
}

void InstructionF64::generateCValue(std::ostream& os, InstructionContext& context)
{
    union
    {
        uint64_t i;
        double f;
    };

    f = value;

    if ((i & 0x7ff0000000000000ull) == 0x7ff0000000000000ull) {
        if ((i & 0x8000000000000000) != 0) {
            os << '-';
            i &= 0x7fffffffffffffff;
        }

        if (i  == 0x7ff0000000000000ull) {
            os << "INFINITY";
        } else {
            if (i != 0x7ff8000000000000ull) {
                os << "nan(";
                os << "\"0x" << std::hex << (i & 0xfffffffffffffull) << std::dec << "\")";
            }
        }
    } else  {
        os << value;
    }
}

InstructionV128* InstructionV128::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionV128>();

    result->value = requiredV128(context);

    return result;
}

InstructionV128* InstructionV128::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionV128>();

    result->value = data.getV128();

    return result;
}

void InstructionV128::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putV128(value);
}

void InstructionV128::check(CheckContext& context)
{
    // nothing to do
}

void InstructionV128::generate(std::ostream& os, InstructionContext& context)
{
    auto flags = os.flags();
    union
    {
        int32_t a32[4];
        v128_t v128;
    };

    os << opcode << " i32x4";

    v128 = value;

    for (int i = 0; i < 4; ++i) {
        os << " 0x" << std::hex << std::setw(8) << std::setfill('0') << a32[i];
    }

    os.flags(flags);
}

InstructionBlock* InstructionBlock::parse(SourceContext& context, Opcode opcode)
{
    auto* module = context.getModule();

    auto* result = context.makeTreeNode<InstructionBlock>();

    if (auto id = context.getId()) {
        result->label = *id;
        module->pushLabel(*id);
    } else {
        module->pushLabel({});
    }

    TypeUse::parse(context, result, true);

    if (result->signature) {
        if (!result->signature->getParams().empty() || result->signature->getResults().size() > 1) {
            result->checkSignature(context);
        } else if (!result->signature->getResults().empty()) {
            result->resultType = result->signature->getResults()[0];
            result->signature.reset(nullptr);
            result->signatureIndex = invalidIndex;
        }
    }

    return result;
}

InstructionBlock* InstructionBlock::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionBlock>();

    auto type = context.data().getI32leb();

    if (type >= 0) {
        result->signatureIndex = type;

        auto* typeDeclaration = context.getModule()->getType(result->signatureIndex);

        result->signature.reset(context.makeTreeNode<Signature>(*typeDeclaration->getSignature()));
    } else {
        result->resultType = type;
    }

    return result;
}

void InstructionBlock::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    if (resultType != ValueType::void_ || signatureIndex == invalidIndex) {
        data.putI32leb(int32_t(resultType));
    } else {
        data.putI32leb(signatureIndex);
    }
}

void InstructionBlock::check(CheckContext& context)
{
    context.checkValueType(this, resultType);
    // TBI multi values
}

void InstructionBlock::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;

    if (resultType != ValueType::void_) {
        os << " (result " << resultType << ')';
    } else if (signatureIndex != invalidIndex) {
        signature->generate(os, context.getModule());
    }

    if (context.getComments()) {
        os << "  ;; label = @" << context.enterBlock();
    } else {
        context.enterBlock();
    }
}

InstructionIdx* InstructionIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionIdx>();

    result->index = requiredU32(context);

    return result;
}

InstructionIdx* InstructionIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionIdx>();

    result->index = context.data().getU32leb();

    return result;
}

void InstructionIdx::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU32leb(index);
}

void InstructionIdx::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << index;
}

InstructionLocalIdx* InstructionLocalIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionLocalIdx>();

    if (auto index = parseLocalIndex(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid local index.");
    }

    return result;
}

InstructionLocalIdx* InstructionLocalIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionLocalIdx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionFunctionIdx* InstructionFunctionIdx::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();
    auto* result = context.makeTreeNode<InstructionFunctionIdx>();

    if (auto index = parseFunctionIndex(context)) {
        result->index = *index;
    } else {
        context.msgs().error(tokens.peekToken(), "Missing or invalid function index.");
    }

    return result;
}

void InstructionFunctionIdx::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " ";

    auto* function = context.getModule()->getFunction(index);

    if (!function->getId().empty()) {
        os << '$' << function->getId();
    } else if (!function->getExternId().empty()) {
        os << index << " (;" << function->getExternId() << ";)";
    } else {
        os << index;
    }
}

InstructionFunctionIdx* InstructionFunctionIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionFunctionIdx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionGlobalIdx* InstructionGlobalIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionGlobalIdx>();

    if (auto index = parseGlobalIndex(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid global index.");
    }

    return result;
}

void InstructionGlobalIdx::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " ";

    auto* global = context.getModule()->getGlobal(index);

    if (!global->getId().empty()) {
        os << '$' << global->getId();
    } else if (!global->getExternId().empty()) {
        os << index << " (;" << global->getExternId() << ";)";
    } else {
        os << index;
    }
}

InstructionGlobalIdx* InstructionGlobalIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionGlobalIdx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionLabelIdx* InstructionLabelIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionLabelIdx>();

    if (auto index = parseLabelIndex(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid label index.");
    }

    return result;
}

InstructionLabelIdx* InstructionLabelIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionLabelIdx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionEventIdx* InstructionEventIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionEventIdx>();

    if (auto index = parseEventIndex(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid event index.");
    }

    return result;
}

InstructionEventIdx* InstructionEventIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionEventIdx>();

    result->index = context.data().getU32leb();

    return result;
}

void InstructionEventIdx::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " ";

    auto* event = context.getModule()->getEvent(index);

    if (!event->getId().empty()) {
        os << '$' << event->getId();
    } else if (!event->getExternId().empty()) {
        os << index << " (;" << event->getExternId() << ";)";
    } else {
        os << index;
    }
}

InstructionSegmentIdx* InstructionSegmentIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionSegmentIdx>();

    if (auto index = parseSegmentIndex(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid data segment index.");
    }

    return result;
}

InstructionSegmentIdx* InstructionSegmentIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionSegmentIdx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionElementIdx* InstructionElementIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionElementIdx>();

    if (auto index = parseElementIndex(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid element index.");
    }

    return result;
}

InstructionElementIdx* InstructionElementIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionElementIdx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionLane2Idx* InstructionLane2Idx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionLane2Idx>();

    if (auto index = parseLane2Index(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid lane index.");
    }

    return result;
}

InstructionLane2Idx* InstructionLane2Idx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionLane2Idx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionLane4Idx* InstructionLane4Idx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionLane4Idx>();

    if (auto index = parseLane4Index(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid lane index.");
    }

    return result;
}

InstructionLane4Idx* InstructionLane4Idx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionLane4Idx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionLane8Idx* InstructionLane8Idx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionLane8Idx>();

    if (auto index = parseLane8Index(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid lane index.");
    }

    return result;
}

InstructionLane8Idx* InstructionLane8Idx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionLane8Idx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionLane16Idx* InstructionLane16Idx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionLane16Idx>();

    if (auto index = parseLane16Index(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid lane index.");
    }

    return result;
}

InstructionLane16Idx* InstructionLane16Idx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionLane16Idx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionLane32Idx* InstructionLane32Idx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionLane32Idx>();

    if (auto index = parseLane32Index(context); index) {
        result->index = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid lane index.");
    }

    return result;
}

InstructionLane32Idx* InstructionLane32Idx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionLane32Idx>();

    result->index = context.data().getU32leb();

    return result;
}

InstructionShuffle* InstructionShuffle::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionShuffle>();

    for (int i = 0; i < 16; ++i) {
        if (auto index = parseLane32Index(context)) {
            result->value[i] = uint8_t(*index);
        } else {
            context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid lane index.");
        }
    }

    return result;
}

InstructionShuffle* InstructionShuffle::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionShuffle>();

    for (int i = 0; i < 16; ++i) {
        result->value[i] = data.getI8();
    }

    return result;
}

void InstructionShuffle::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);

    for (int i = 0; i < 16; ++i) {
        data.putI8(value[i]);
    }
}

void InstructionShuffle::check(CheckContext& context)
{
    // nothing to do
}

void InstructionShuffle::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;

    for (int i = 0; i < 16; ++i) {
        os << "  " << int32_t(value[i]);
    }
}

InstructionBrTable* InstructionBrTable::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();

    auto* result = context.makeTreeNode<InstructionBrTable>();

    while (auto label = parseLabelIndex(context)) {
        result->labels.push_back(*label);
    }

    if (result->labels.empty()) {
        context.msgs().error(tokens.peekToken(), "Missing or invalid label");
    } else {
        result->defaultLabel = result->labels.back();
        result->labels.pop_back();
    }

    return result;
}

InstructionBrTable* InstructionBrTable::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionBrTable>();

    for (auto length = data.getU32leb(); length > 0; --length) {
        result->labels.push_back(data.getU32leb());
    }

    result->defaultLabel = data.getU32leb();
    return result;
}

void InstructionBrTable::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU32leb(uint32_t(labels.size()));

    for (auto label : labels) {
        data.putU32leb(label);
    }

    data.putU32leb(defaultLabel);
}

void InstructionBrTable::check(CheckContext& context)
{
    // nothing to do
}

void InstructionBrTable::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;

    for (auto label : labels) {
        os << " " << label;
    }

    os << " " << defaultLabel;
}

InstructionMemory* InstructionMemory::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();

    auto* result = context.makeTreeNode<InstructionMemory>();

    if (tokens.getKeyword("offset=")) {
        result->offset = requiredU32(context);
    }

    uint32_t align = opcode.getAlign();

    if (tokens.getKeyword("align=")) {
        align = requiredU32(context);
    }

    uint32_t power = 0;

    if (align != 0) {
        while ((align & 1) == 0) {
            power++;
            align >>= 1;
        }

        context.msgs().errorWhen(align != 1, tokens.peekToken(-1), "Alignment must be a power of 2.");
    }

    result->alignPower = power;

    return result;
}

InstructionMemory* InstructionMemory::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionMemory>();

    result->alignPower = data.getU32leb();
    result->offset = data.getU32leb();

    return result;
}

void InstructionMemory::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU32leb(alignPower);
    data.putU32leb(offset);
}

void InstructionMemory::check(CheckContext& context)
{
    // nothing to do
}

void InstructionMemory::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;

    if (offset != 0) {
        os << " offset=" << offset;
    }

    auto align = 1u << alignPower;

    if (align != opcode.getAlign()) {
        os << " align=" << align;
    }
}

InstructionIndirect* InstructionIndirect::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionIndirect>();

    TypeUse typeUse;

    if (auto table = parseTableIndex(context)) {
        result->tableIndex = uint8_t(*table);
    }

    TypeUse::parse(context, &typeUse);

    result->typeIndex = typeUse.getSignatureIndex();

    return result;
}

InstructionIndirect* InstructionIndirect::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionIndirect>();

    result->typeIndex = context.data().getU32leb();
    result->tableIndex = context.data().getU8();

    return result;
}

void InstructionIndirect::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU32leb(typeIndex);
    data.putU8(tableIndex);
}

void InstructionIndirect::check(CheckContext& context)
{
    context.checkTypeIndex(this, typeIndex);

    auto* type = context.getModule()->getType(typeIndex);

    type->setUsedAsIndirect(true);
}

void InstructionIndirect::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
    if (tableIndex != 0) {
        os << ' ' << uint32_t(tableIndex);
    }

    os << " (type " << typeIndex << ')';
}

InstructionDepthEventIdx* InstructionDepthEventIdx::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionDepthEventIdx>();

    result->depth = requiredI32(context);
    if (auto index = parseEventIndex(context); index) {
        result->eventIndex = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid event index.");
    }

    return result;
}

InstructionDepthEventIdx* InstructionDepthEventIdx::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionDepthEventIdx>();

    result->depth = context.data().getU32leb();
    result->eventIndex = context.data().getU32leb();

    return result;
}

void InstructionDepthEventIdx::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU32leb(depth);
    data.putU32leb(eventIndex);
}

void InstructionDepthEventIdx::check(CheckContext& context)
{
    context.checkEventIndex(this, eventIndex);
}

void InstructionDepthEventIdx::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " (type " << eventIndex << ')';
}

InstructionSegmentIdxMem* InstructionSegmentIdxMem::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionSegmentIdxMem>();

    if (auto index = parseSegmentIndex(context); index) {
        result->segmentIndex = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid data segment index.");
    }

    return result;
}

InstructionSegmentIdxMem* InstructionSegmentIdxMem::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionSegmentIdxMem>();

    result->segmentIndex = data.getU32leb();
    result->memory = data.getU8();

    return result;
}

void InstructionSegmentIdxMem::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU32leb(segmentIndex);
    data.putU8(memory);
}

void InstructionSegmentIdxMem::check(CheckContext& context)
{
    // nothing to do
}

void InstructionSegmentIdxMem::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << segmentIndex;
}

InstructionMem0* InstructionMem0::parse(SourceContext& context, Opcode opcode)
{
    return context.makeTreeNode<InstructionMem0>();
}

InstructionMem0* InstructionMem0::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionMem0>();

    context.msgs().errorWhen(data.getU8() != 0, "reserved argument must be 0.");
    return result ;
}

void InstructionMem0::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU8(0);
}

void InstructionMem0::check(CheckContext& context)
{
    // nothing to do
}

void InstructionMem0::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
}

InstructionMem* InstructionMem::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionMem>();

    return result;
}

InstructionMem* InstructionMem::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionMem>();

    result->memory = data.getU8();

    return result;
}

void InstructionMem::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU8(memory);
}

void InstructionMem::check(CheckContext& context)
{
    // nothing to do
}

void InstructionMem::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
}

InstructionMemMem* InstructionMemMem::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionMemMem>();

    return result;
}

InstructionMemMem* InstructionMemMem::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionMemMem>();

    result->destination = data.getU8();
    result->source = data.getU8();

    return result;
}

void InstructionMemMem::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU8(destination);
    data.putU8(source);
}

void InstructionMemMem::check(CheckContext& context)
{
    // nothing to do
}

void InstructionMemMem::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
}

InstructionTableElementIdx* InstructionTableElementIdx::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto& msgs = context.msgs();
    auto* result = context.makeTreeNode<InstructionTableElementIdx>();

    if (auto value = tokens.getU32()) {
        if (auto index = parseElementIndex(context); index) {
            msgs.errorWhen((*value >= module->getTableCount()), tokens.peekToken(-2),
                    "Invalid table index ", *value, '.');

            result->tableIndex = uint8_t(*value);
            result->elementIndex = *index;
        } else {
            msgs.errorWhen((*value >= module->getElementCount()), tokens.peekToken(-1),
                    "Invalid element index ", *value, '.');

            result->elementIndex = *value;
        }
    } else {
        if (auto table = parseTableIndex(context)) {
            result->tableIndex = uint8_t(*table);
        }

        if (auto index = parseElementIndex(context); index) {
            result->elementIndex = *index;
        } else {
            context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid element index.");
        }
    }

    return result;
}

InstructionTableElementIdx* InstructionTableElementIdx::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionTableElementIdx>();

    result->elementIndex = data.getU32leb();
    result->tableIndex = data.getU8();

    return result;
}

void InstructionTableElementIdx::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU32leb(elementIndex);
    data.putU8(tableIndex);
}

void InstructionTableElementIdx::check(CheckContext& context)
{
    // nothing to do
}

void InstructionTableElementIdx::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << ' ' << uint32_t(tableIndex) << ' ' << elementIndex;
}

InstructionTable* InstructionTable::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionTable>();

    if (auto table = parseTableIndex(context)) {
        result->tableIndex = uint8_t(*table);
    }

    return result;
}

InstructionTable* InstructionTable::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionTable>();

    result->tableIndex = data.getU8();

    return result;
}

void InstructionTable::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU8(tableIndex);
}

void InstructionTable::check(CheckContext& context)
{
    // nothing to do
}

void InstructionTable::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
}

InstructionTableTable* InstructionTableTable::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionTableTable>();

    if (auto table = parseTableIndex(context)) {
        result->destination = uint8_t(*table);
        if (auto table = parseTableIndex(context)) {
            result->source = uint8_t(*table);
        }
    }

    return result;
}

InstructionTableTable* InstructionTableTable::read(BinaryContext& context)
{
    auto& data = context.data();
    auto* result = context.makeTreeNode<InstructionTableTable>();

    result->destination = data.getU8();
    result->source = data.getU8();

    return result;
}

void InstructionTableTable::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putU8(destination);
    data.putU8(source);
}

void InstructionTableTable::check(CheckContext& context)
{
    // nothing to do
}

void InstructionTableTable::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
}

InstructionRefType* InstructionRefType::parse(SourceContext& context, Opcode opcode)
{
    auto* result = context.makeTreeNode<InstructionRefType>();

    result->type = requiredRefType(context);

    return result;
}

InstructionRefType* InstructionRefType::read(BinaryContext& context)
{
    auto* result = context.makeTreeNode<InstructionRefType>();

    result->type = context.data().getI32leb();

    return result;
}

void InstructionRefType::write(BinaryContext& context)
{
    auto& data = context.data();

    writeOpcode(context);
    data.putI32leb(type);
}

void InstructionRefType::check(CheckContext& context)
{
    context.checkValueType(this, type);
}

void InstructionRefType::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << type.getRefName();
}

Instruction* Instruction::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    auto token = tokens.getKeyword();

    if (!token) {
        return nullptr;
    }

    auto opcode = Opcode::fromString(*token);

    if (!opcode) {
        tokens.bump(-1);
        return nullptr;
    }

    if (opcode == Opcode::memory__init || opcode == Opcode::data__drop) {
        context.getModule()->setDataCountNeeded();
    }

    Instruction* result = nullptr;

    auto encoding = opcode->getImmediateType();

    switch(encoding) {
        case ImmediateType::none:               result = InstructionNone::parse(context, *opcode); break;
        case ImmediateType::select:             result = InstructionSelect::parse(context, *opcode); break;
        case ImmediateType::i32:                result = InstructionI32::parse(context, *opcode); break;
        case ImmediateType::i64:                result = InstructionI64::parse(context, *opcode); break;
        case ImmediateType::f32:                result = InstructionF32::parse(context, *opcode); break;
        case ImmediateType::f64:                result = InstructionF64::parse(context, *opcode); break;
        case ImmediateType::v128:               result = InstructionV128::parse(context, *opcode); break;
        case ImmediateType::block:              result = InstructionBlock::parse(context, *opcode); break;
        case ImmediateType::idx:                result = InstructionIdx::parse(context, *opcode); break;
        case ImmediateType::localIdx:           result = InstructionLocalIdx::parse(context, *opcode); break;
        case ImmediateType::globalIdx:          result = InstructionGlobalIdx::parse(context, *opcode); break;
        case ImmediateType::elementIdx:         result = InstructionElementIdx::parse(context, *opcode); break;
        case ImmediateType::segmentIdx:         result = InstructionSegmentIdx::parse(context, *opcode); break;
        case ImmediateType::segmentIdxMem:      result = InstructionSegmentIdxMem::parse(context, *opcode); break;
        case ImmediateType::mem:                result = InstructionMem::parse(context, *opcode); break;
        case ImmediateType::table:              result = InstructionTable::parse(context, *opcode); break;
        case ImmediateType::memMem:             result = InstructionMemMem::parse(context, *opcode); break;
        case ImmediateType::tableElementIdx:    result = InstructionTableElementIdx::parse(context, *opcode); break;
        case ImmediateType::tableTable:         result = InstructionTableTable::parse(context, *opcode); break;
        case ImmediateType::functionIdx:        result = InstructionFunctionIdx::parse(context, *opcode); break;
        case ImmediateType::labelIdx:           result = InstructionLabelIdx::parse(context, *opcode); break;
        case ImmediateType::lane2Idx:           result = InstructionLane2Idx::parse(context, *opcode); break;
        case ImmediateType::lane4Idx:           result = InstructionLane4Idx::parse(context, *opcode); break;
        case ImmediateType::lane8Idx:           result = InstructionLane8Idx::parse(context, *opcode); break;
        case ImmediateType::lane16Idx:          result = InstructionLane16Idx::parse(context, *opcode); break;
        case ImmediateType::lane32Idx:          result = InstructionLane32Idx::parse(context, *opcode); break;
        case ImmediateType::shuffle:            result = InstructionShuffle::parse(context, *opcode); break;
        case ImmediateType::brTable:            result = InstructionBrTable::parse(context, *opcode); break;
        case ImmediateType::eventIdx:           result = InstructionEventIdx::parse(context, *opcode); break;
        case ImmediateType::depthEventIdx:      result = InstructionDepthEventIdx::parse(context, *opcode); break;
        case ImmediateType::memory:             result = InstructionMemory::parse(context, *opcode); break;
        case ImmediateType::mem0:               result = InstructionMem0::parse(context, *opcode); break;
        case ImmediateType::indirect:           result = InstructionIndirect::parse(context, *opcode); break;
        case ImmediateType::refType:            result = InstructionRefType::parse(context, *opcode); break;
        default:
            context.msgs().error(tokens.peekToken(-1), "Invalid encoding ", unsigned(encoding));
            return nullptr;
    }

    result->setOpcode(*opcode);
    return result;
}

bool Instruction::parseFolded(SourceContext& context, std::vector<Instruction*>& instructions)
{
    auto& tokens = context.tokens();

    if (tokens.getParenthesis('(')) {
        auto& msgs = context.msgs();

        if (auto instruction0 = parse(context); instruction0 != nullptr) {
            if (instruction0->getOpcode() == Opcode::block ||
                    instruction0->getOpcode() == Opcode::loop) {
                instructions.push_back(instruction0);
                parse(context, instructions);
                instructions.push_back(context.makeTreeNode<InstructionNone>(Opcode(Opcode::end)));
                context.getModule()->popLabel();
            } else if (instruction0->getOpcode() == Opcode::if_) {
                while (parseFolded(context, instructions)) {
                    // nop
                }

                instructions.push_back(instruction0);

                if (startClause(context, "then")) {
                    parse(context, instructions);

                    if (!requiredParenthesis(context, ')')) {
                        tokens.recover();
                    }
                }

                if (startClause(context, "else")) {
                    if (!tokens.getParenthesis(')')) {
                        instructions.push_back(context.makeTreeNode<InstructionNone>(Opcode(Opcode::else_)));
                        parse(context, instructions);

                        if (!requiredParenthesis(context, ')')) {
                            tokens.recover();
                        }
                    }
                }

                instructions.push_back(context.makeTreeNode<InstructionNone>(Opcode(Opcode::end)));
                context.getModule()->popLabel();
            } else {
                while (parseFolded(context, instructions)) {
                    // nop
                }

                instructions.push_back(instruction0);
            }
        } else if (tokens.getKeyword("then")) {
            tokens.bump(-2);
            return false;
        } else {
            msgs.error(tokens.peekToken(-1), "Missing or invald instruction in fold.");
        }

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
        }

        return true;
    }

    return false;
}

void Instruction::parse(SourceContext& context, std::vector<Instruction*>& instructions)
{
    for (;;) {
        if (auto instruction = parse(context); instruction != nullptr) {
            instructions.emplace_back(instruction);
        } else if (parseFolded(context, instructions)) {
            // nop
        } else {
            break;
        }
    }
}

Opcode Instruction::readOpcode(BinaryContext& context)
{
    auto& data = context.data();

    if (OpcodePrefix prefix = OpcodePrefix(data.getU8()); prefix.isValid()) {
        return Opcode((uint8_t(prefix) << 24) | data.getU32leb());
    } else {
        return Opcode(uint8_t(prefix));
    }
}

Instruction* Instruction::read(BinaryContext& context)
{
    Instruction* result = nullptr;

    auto opcode = readOpcode(context);

    auto encoding = opcode.getImmediateType();

    switch(encoding) {
        case ImmediateType::none:               result = InstructionNone::read(context); break;
        case ImmediateType::select:             result = InstructionSelect::read(context, opcode); break;
        case ImmediateType::i32:                result = InstructionI32::read(context); break;
        case ImmediateType::i64:                result = InstructionI64::read(context); break;
        case ImmediateType::f32:                result = InstructionF32::read(context); break;
        case ImmediateType::f64:                result = InstructionF64::read(context); break;
        case ImmediateType::v128:               result = InstructionV128::read(context); break;
        case ImmediateType::block:              result = InstructionBlock::read(context); break;
        case ImmediateType::idx:                result = InstructionIdx::read(context); break;
        case ImmediateType::localIdx:           result = InstructionLocalIdx::read(context); break;
        case ImmediateType::globalIdx:          result = InstructionGlobalIdx::read(context); break;
        case ImmediateType::elementIdx:         result = InstructionElementIdx::read(context); break;
        case ImmediateType::segmentIdx:         result = InstructionSegmentIdx::read(context); break;
        case ImmediateType::segmentIdxMem:      result = InstructionSegmentIdxMem::read(context); break;
        case ImmediateType::mem:                result = InstructionMem::read(context); break;
        case ImmediateType::table:              result = InstructionTable::read(context); break;
        case ImmediateType::memMem:             result = InstructionMemMem::read(context); break;
        case ImmediateType::tableElementIdx:    result = InstructionTableElementIdx::read(context); break;
        case ImmediateType::tableTable:         result = InstructionTableTable::read(context); break;
        case ImmediateType::functionIdx:        result = InstructionFunctionIdx::read(context); break;
        case ImmediateType::labelIdx:           result = InstructionLabelIdx::read(context); break;
        case ImmediateType::lane2Idx:           result = InstructionLane2Idx::read(context); break;
        case ImmediateType::lane4Idx:           result = InstructionLane4Idx::read(context); break;
        case ImmediateType::lane8Idx:           result = InstructionLane8Idx::read(context); break;
        case ImmediateType::lane16Idx:          result = InstructionLane16Idx::read(context); break;
        case ImmediateType::lane32Idx:          result = InstructionLane32Idx::read(context); break;
        case ImmediateType::shuffle:            result = InstructionShuffle::read(context); break;
        case ImmediateType::brTable:            result = InstructionBrTable::read(context); break;
        case ImmediateType::depthEventIdx:      result = InstructionDepthEventIdx::read(context); break;
        case ImmediateType::memory:             result = InstructionMemory::read(context); break;
        case ImmediateType::mem0:               result = InstructionMem0::read(context); break;
        case ImmediateType::indirect:           result = InstructionIndirect::read(context); break;
        case ImmediateType::refType:            result = InstructionRefType::read(context); break;
        default:
            context.msgs().error("Invalid encoding ", unsigned(encoding));
            return nullptr;
    }

    result->setOpcode(opcode);
    return result;
}

};
