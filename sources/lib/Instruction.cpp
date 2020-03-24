// Instruction.cpp

#include "Instruction.h"

#include "BackBone.h"
#include "TokenBuffer.h"
#include "parser.h"

InstructionNone* InstructionNone::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();

    if (opcode == Opcode::end || opcode == Opcode::else_) {
        if (auto id = tokens.getId()) {
            auto index = context.getLabelIndex(*id);

            context.msgs().errorWhen(index != 0, tokens.peekToken(-1),
                    "Id must be equal to the label of the innermost block.");
        }
    }

    if (opcode == Opcode::end) {
        context.popLabel();
    }

    return new InstructionNone();
}

InstructionNone* InstructionNone::read(BinaryContext& context)
{
    auto result = new InstructionNone();

    return result;
}

void InstructionNone::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
}

void InstructionNone::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
    if (opcode == Opcode::end) {
        context.leaveBlock();
    }
}

InstructionI32* InstructionI32::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionI32();

    result->imm = requiredI32(context);

    return result;
}

InstructionI32* InstructionI32::read(BinaryContext& context)
{
    auto result = new InstructionI32();

    result->imm = context.data().getI32leb();

    return result;
}

void InstructionI32::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putI32leb(imm);
}

void InstructionI32::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << imm;
}

InstructionI64* InstructionI64::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionI64();

    result->imm = requiredI64(context);

    return result;
}

InstructionI64* InstructionI64::read(BinaryContext& context)
{
    auto result = new InstructionI64();

    result->imm = context.data().getI64leb();

    return result;
}

void InstructionI64::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putI64leb(imm);
}

void InstructionI64::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << imm;
}

InstructionF32* InstructionF32::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionF32();

    result->imm = requiredF32(context);

    return result;
}

InstructionF32* InstructionF32::read(BinaryContext& context)
{
    auto result = new InstructionF32();

    result->imm = context.data().getF();

    return result;
}

void InstructionF32::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putF(imm);
}

void InstructionF32::generate(std::ostream& os, InstructionContext& context)
{
    auto flags = os.flags();

    os << opcode << " " << std::hexfloat << imm << std::defaultfloat << " (;=" << imm << ";)";
    os.flags(flags);
}

InstructionF64* InstructionF64::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionF64();

    result->imm = requiredF64(context);

    return result;
}

InstructionF64* InstructionF64::read(BinaryContext& context)
{
    auto result = new InstructionF64();

    result->imm = context.data().getD();

    return result;
}

void InstructionF64::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putD(imm);
}

void InstructionF64::generate(std::ostream& os, InstructionContext& context)
{
    auto flags = os.flags();

    os << opcode << " " << std::hexfloat << imm << std::defaultfloat << " (;=" << imm << ";)";
    os.flags(flags);
}

InstructionBlock* InstructionBlock::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();

    auto result = new InstructionBlock();

    if (auto id = tokens.getId()) {
        result->label = *id;
        context.pushLabel(*id);
    } else {
        context.pushLabel({});
    }

    if (startClause(context, "result")) {
        if (auto value = parseValueType(context)) {
            result->imm = *value;
        } else {
            context.msgs().error(tokens.peekToken(), "Missing or invalid result type.");
        }

        if (!requiredParenthesis(context, ')')) {
            tokens.recover();
        }
    }

    if (auto value = parseValueType(context)) {
        result->imm = *value;
    }

    return result;
}

InstructionBlock* InstructionBlock::read(BinaryContext& context)
{
    auto result = new InstructionBlock();

    result->imm = ValueType(context.data().getI32leb());

    return result;
}

void InstructionBlock::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putI32leb(int32_t(imm));
}

void InstructionBlock::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;

    if (imm != ValueType::void_) {
        os << " (result " << imm << ')';
    }

    os << "  ;; label = @" << context.enterBlock();
}

InstructionIdx* InstructionIdx::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionIdx();

    result->imm = requiredU32(context);

    return result;
}

InstructionIdx* InstructionIdx::read(BinaryContext& context)
{
    auto result = new InstructionIdx();

    result->imm = context.data().getU32leb();

    return result;
}

void InstructionIdx::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putU32leb(imm);
}

void InstructionIdx::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " " << imm;
}

InstructionLocalIdx* InstructionLocalIdx::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionLocalIdx();

    if (auto index = parseLocalIndex(context); index) {
        result->imm = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid local index.");
    }

    return result;
}

InstructionLocalIdx* InstructionLocalIdx::read(BinaryContext& context)
{
    auto result = new InstructionLocalIdx();

    result->imm = context.data().getU32leb();

    return result;
}

InstructionFunctionIdx* InstructionFunctionIdx::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();
    auto& token = tokens.peekToken();
    auto result = new InstructionFunctionIdx();

    if (auto index = parseFunctionIndex(context)) {
        result->imm = *index;
    } else {
        context.msgs().error(tokens.peekToken(), "Missing or invalid function index.");
    }

    return result;
}

InstructionFunctionIdx* InstructionFunctionIdx::read(BinaryContext& context)
{
    auto result = new InstructionFunctionIdx();

    result->imm = context.data().getU32leb();

    return result;
}

InstructionGlobalIdx* InstructionGlobalIdx::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionGlobalIdx();

    if (auto index = parseGlobalIndex(context); index) {
        result->imm = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid global index.");
    }

    return result;
}

InstructionGlobalIdx* InstructionGlobalIdx::read(BinaryContext& context)
{
    auto result = new InstructionGlobalIdx();

    result->imm = context.data().getU32leb();

    return result;
}

InstructionLabelIdx* InstructionLabelIdx::parse(SourceContext& context, Opcode opcode)
{
    auto result = new InstructionLabelIdx();

    if (auto index = parseLabelIndex(context); index) {
        result->imm = *index;
    } else {
        context.msgs().error(context.tokens().peekToken(-1), "Missing or invalid local index.");
    }

    return result;
}

InstructionLabelIdx* InstructionLabelIdx::read(BinaryContext& context)
{
    auto result = new InstructionLabelIdx();

    result->imm = context.data().getU32leb();

    return result;
}

InstructionTable* InstructionTable::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();

    auto result = new InstructionTable();

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

InstructionTable* InstructionTable::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new InstructionTable();

    for (auto length = data.getU32leb(); length > 0; --length) {
        result->labels.push_back(data.getU32leb());
    }

    result->defaultLabel = data.getU32leb();
    return result;
}

void InstructionTable::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putU32leb(uint32_t(labels.size()));

    for (auto label : labels) {
        data.putU32leb(label);
    }

    data.putU32leb(defaultLabel);
}

void InstructionTable::generate(std::ostream& os, InstructionContext& context)
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

    auto result = new InstructionMemory();

    if (tokens.getKeyword("offset=")) {
        result->offset = requiredU32(context);
    }

    if (tokens.getKeyword("align=")) {
        uint32_t align = requiredU32(context);
        uint32_t power = 0;

        if (align != 0) {
            while ((align & 1) == 0) {
                power++;
                align >>= 1;
            }

            context.msgs().errorWhen(align != 1, tokens.peekToken(-1), "Alignment must be a power of 2.");
        }

        result->alignPower = power;
    } else {
        result->alignPower = opcode.getAlign();
    }

    return result;
}

InstructionMemory* InstructionMemory::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new InstructionMemory();

    result->alignPower = data.getU32leb();
    result->offset = data.getU32leb();

    return result;
}

void InstructionMemory::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putU32leb(alignPower);
    data.putU32leb(offset);
}

void InstructionMemory::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;

    if (offset != 0) {
        os << " offset=" << offset;
    }

    if (alignPower != opcode.getAlign()) {
        os << " align=" << (1 << alignPower);
    }
}

InstructionMemory0* InstructionMemory0::parse(SourceContext& context, Opcode opcode)
{
    return new InstructionMemory0();
}

InstructionMemory0* InstructionMemory0::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new InstructionMemory0();

    context.msgs().errorWhen(data.getU8() != 0, "reserved argument must be 0.");
    return result ;
}

InstructionIndirect* InstructionIndirect::parse(SourceContext& context, Opcode opcode)
{
    auto& tokens = context.tokens();
    auto result = new InstructionIndirect();

    TypeUse typeUse;

    TypeUse::parse(context, &typeUse);

    result->typeIndex = typeUse.getSignatureIndex();

    return result;
}

InstructionIndirect* InstructionIndirect::read(BinaryContext& context)
{
    auto result = new InstructionIndirect();

    result->typeIndex = context.data().getU32leb();
    result->dummy = context.data().getU32leb();

    return result;
}

void InstructionIndirect::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putU32leb(typeIndex);
    data.putU32leb(dummy);
}

void InstructionIndirect::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode << " (type " << typeIndex << ')';
}

void InstructionMemory0::write(BinaryContext& context)
{
    auto& data = context.data();

    data.putU8(uint8_t(opcode));
    data.putU8(0);
}

void InstructionMemory0::generate(std::ostream& os, InstructionContext& context)
{
    os << opcode;
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

    auto result = new Instruction();

    auto encoding = opcode->getParameterEncoding();

    switch(encoding) {
        case ParameterEncoding::none:          result = InstructionNone::parse(context, *opcode); break;
        case ParameterEncoding::i32:           result = InstructionI32::parse(context, *opcode); break;
        case ParameterEncoding::i64:           result = InstructionI64::parse(context, *opcode); break;
        case ParameterEncoding::f32:           result = InstructionF32::parse(context, *opcode); break;
        case ParameterEncoding::f64:           result = InstructionF64::parse(context, *opcode); break;
        case ParameterEncoding::block:         result = InstructionBlock::parse(context, *opcode); break;
        case ParameterEncoding::idx:           result = InstructionIdx::parse(context, *opcode); break;
        case ParameterEncoding::localIdx:      result = InstructionLocalIdx::parse(context, *opcode); break;
        case ParameterEncoding::globalIdx:     result = InstructionGlobalIdx::parse(context, *opcode); break;
        case ParameterEncoding::functionIdx:   result = InstructionFunctionIdx::parse(context, *opcode); break;
        case ParameterEncoding::labelIdx:      result = InstructionLabelIdx::parse(context, *opcode); break;
        case ParameterEncoding::table:         result = InstructionTable::parse(context, *opcode); break;
        case ParameterEncoding::memory:        result = InstructionMemory::parse(context, *opcode); break;
        case ParameterEncoding::memory0:       result = InstructionMemory0::parse(context, *opcode); break;
        case ParameterEncoding::indirect:      result = InstructionIndirect::parse(context, *opcode); break;
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
                instructions.push_back(new InstructionNone(Opcode(Opcode::end)));
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
                    instructions.push_back(new InstructionNone(Opcode(Opcode::else_)));
                    parse(context, instructions);

                    if (!requiredParenthesis(context, ')')) {
                        tokens.recover();
                    }
                }

                instructions.push_back(new InstructionNone(Opcode(Opcode::end)));
            } else {
                while (parseFolded(context, instructions)) {
                    // nop
                }

                instructions.push_back(instruction0);
            }
        } else if (auto key = tokens.getKeyword("then")) {
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
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

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

Instruction* Instruction::read(BinaryContext& context)
{
    auto& data = context.data();
    auto result = new Instruction();

    Opcode opcode(data.getU8());
    auto encoding = opcode.getParameterEncoding();

    switch(encoding) {
        case ParameterEncoding::none:          result = InstructionNone::read(context); break;
        case ParameterEncoding::i32:           result = InstructionI32::read(context); break;
        case ParameterEncoding::i64:           result = InstructionI64::read(context); break;
        case ParameterEncoding::f32:           result = InstructionF32::read(context); break;
        case ParameterEncoding::f64:           result = InstructionF64::read(context); break;
        case ParameterEncoding::block:         result = InstructionBlock::read(context); break;
        case ParameterEncoding::idx:           result = InstructionIdx::read(context); break;
        case ParameterEncoding::localIdx:      result = InstructionLocalIdx::read(context); break;
        case ParameterEncoding::globalIdx:     result = InstructionGlobalIdx::read(context); break;
        case ParameterEncoding::functionIdx:   result = InstructionFunctionIdx::read(context); break;
        case ParameterEncoding::labelIdx:      result = InstructionLabelIdx::read(context); break;
        case ParameterEncoding::table:         result = InstructionTable::read(context); break;
        case ParameterEncoding::memory:        result = InstructionMemory::read(context); break;
        case ParameterEncoding::memory0:       result = InstructionMemory0::read(context); break;
        case ParameterEncoding::indirect:      result = InstructionIndirect::read(context); break;
        default:
            context.msgs().error("Invalid encoding ", unsigned(encoding));
            return nullptr;
    }

    result->setOpcode(opcode);
    return result;
}

