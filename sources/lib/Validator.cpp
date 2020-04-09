// Validator.cpp

#include "Validator.h"

#include "BackBone.h"
#include "Context.h"
#include "Instruction.h"
#include "Module.h"

namespace libwasm
{

void validate(CheckContext& context)
{
    Validator checker(context);
    auto* module = context.getModule();
    if (auto* codeSection = module->getCodeSection(); codeSection != nullptr) {
        auto& codeEntries = codeSection->getCodes();

        for (auto& code : codeEntries) {
            checker.check(code.get());
        }
    }
}

Validator::Frame::Frame(const std::vector<ValueType>& labelTypes,
        const std::vector<ValueType>& endTypes)
  : labelTypes(labelTypes), endTypes(endTypes)
{
}

Validator::Validator(CheckContext& c)
  : context(c), module(context.getModule()), msgs(c.msgs())
{
}

void Validator::reset()
{
    operands.clear();
    frames.clear();
}

bool Validator::underflow()
{
    if (operands.size() <= frames.back().height && !frames.back().unreachable) {
        msgs.error(currentInstruction,
                "Stack underflow.  Attempt to access stack outside block.");
        return true;
    } else {
        return false;
    }
}


void Validator::pushOperand(ValueType type)
{
    operands.push_back(type);
}

ValueType Validator::popOperand()
{
    if (operands.size() == frames.back().height && frames.back().unreachable) {
        return ValueType::void_;
    }

    if (!underflow()) {
        auto result = operands.back();

        operands.pop_back();
        return result;
    }

    return ValueType::void_;
}

ValueType Validator::popOperand(ValueType expect)
{
    auto actual = popOperand();

    if (actual == ValueType::void_) {
        return expect;
    }

    if (expect == ValueType::void_) {
        return actual;
    }

    msgs.errorWhen((actual != expect), currentInstruction,
            "Invalid stack access.  Expected '", expect, "'; found '", actual, "'.");

    return actual;
}

void Validator::peekOperand(ValueType expect, size_t index)
{
    if (index >= operands.size() - frames.back().height) {
        if (!frames.back().unreachable) {
            msgs.error(currentInstruction,
                    "Stack underflow.  Attempt to access stack outside block.");
        }

        return;
    }

   auto actual = operands[operands.size() - index - 1];

   msgs.errorWhen((actual != expect), currentInstruction,
            "Invalid stack access.  Expected '", expect, "'; found '", actual, "'.");
}

void Validator::pushOperands(const std::vector<ValueType>& types)
{
    for (auto type : types) {
        pushOperand(type);
    }
}

void Validator::popOperands(const std::vector<ValueType>& types)
{
    for (auto i = types.size(); i-- > 0; ) {
        popOperand(types[i]);
    }
}

void Validator::popOperands(const std::vector<std::unique_ptr<Local>>& locals)
{
    for (auto i = locals.size(); i-- > 0; ) {
        popOperand(locals[i]->getType());
    }
}

void Validator::check(CodeEntry* code)
{
    currentSignature = module->getFunction(code->getNumber())->getSignature();

    reset();
    currentCodeEntry = code;
    pushFrame({}, currentSignature->getResults());

    for (auto& instruction : code->getExpression()->getInstructions()) {
        check(instruction.get());
    }
}

void Validator::pushFrame(const std::vector<ValueType>& labelTypes,
        const std::vector<ValueType>& endTypes)
{
    frames.emplace_back(labelTypes, endTypes);

    auto& frame = frames.back();

    frame.height = operands.size();
}

std::vector<ValueType> Validator::popFrame()
{
    if (frames.empty()) {
        msgs.error(currentInstruction, "Frame stack underflow.  Misplaced '",
                currentInstruction->getOpcode(), "'.");
        return {};
    }

    auto& frame = frames.back();

    auto endTypes = frame.endTypes;
    popOperands(endTypes);

    if (operands.size() != frame.height && !frames.back().unreachable) {
        msgs.error(currentInstruction, "Not all pushed operands where consumed.");
    }

    operands.resize(frame.height);
    frames.pop_back();

    return endTypes;
}

const Validator::Frame& Validator::getFrame(size_t index)
{
    return frames[frames.size() - index - 1];
}

void Validator::unreachable()
{
    auto& frame = frames.back();

    operands.resize(frame.height);
    frame.unreachable = true;
}

void Validator::checkBr()
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(currentInstruction);
    auto index = branchInstruction->getIndex();

    msgs.errorWhen((frames.size() < index), currentInstruction,
            "Invald label index '", index, "'; block depth is '", frames.size() - 1, "'.");
    popOperands(getFrame(index).labelTypes);
    unreachable();
}

void Validator::checkBrIf()
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(currentInstruction);
    auto index = branchInstruction->getIndex();

    msgs.errorWhen((frames.size() < index), currentInstruction,
            "Invald label index '", index, "'; block depth is '", frames.size() - 1, "'.");
    popOperand(ValueType::i32);
    popOperands(getFrame(index).labelTypes);
    pushOperands(getFrame(index).labelTypes);
}

void Validator::checkBrTable()
{
    auto* branchInstruction = static_cast<InstructionTable*>(currentInstruction);
    auto defaultIndex = branchInstruction->getDefaultLabel();
    const auto& defaultTypes = getFrame(defaultIndex).labelTypes;

    msgs.errorWhen((frames.size() < defaultIndex), currentInstruction,
            "Invald label index '", defaultIndex, "'; block depth is '", frames.size() - 1, "'.");

    for (auto labelIndex : branchInstruction->getLabels()) {
        msgs.errorWhen((frames.size() < labelIndex), currentInstruction,
                "Invald label index '", labelIndex, "'; block depth is '", frames.size() - 1, "'.");
        msgs.errorWhen((getFrame(labelIndex).labelTypes != defaultTypes), currentInstruction,
                "Inconsistent label '", labelIndex, ",; types are different from default label.");
    }

    popOperand(ValueType::i32);
    popOperands(defaultTypes);
    unreachable();
}

void Validator::checkCall()
{
    auto* callInstruction = static_cast<InstructionFunctionIdx*>(currentInstruction);
    auto* signature = module->getFunction(callInstruction->getIndex())->getSignature();

    popOperands(signature->getParams());
    pushOperands(signature->getResults());
}

void Validator::checkCallIndirect()
{
    auto& types = module->getTypeSection()->getTypes();
    auto* indirectInstruction = static_cast<InstructionIndirect*>(currentInstruction);
    size_t typeIndex = indirectInstruction->getIndex();
    auto* signature = types[typeIndex]->getSignature();

    popOperand(ValueType::i32);

    popOperands(signature->getParams());
    pushOperands(signature->getResults());
}

void Validator::checkBlock()
{
    auto* blockInstruction = static_cast<InstructionBlock*>(currentInstruction);
    std::vector<ValueType> types;
    auto resultType = blockInstruction->getResultType();

    if (resultType != ValueType::void_) {
        types.push_back(blockInstruction->getResultType());
    }

    switch(uint32_t(currentInstruction->getOpcode())) {
        case Opcode::block:
            pushFrame(types, types);

            break;

        case Opcode::loop:
            pushFrame({}, types);

            break;

        case Opcode::try_:
            pushFrame(types, types);

            break;

        case Opcode::if_:
            popOperand(ValueType::i32);
            pushFrame(types, types);

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in checkSpecial.\n";
    }
}

void Validator::checkLocal()
{
    auto* localInstruction = static_cast<InstructionLocalIdx*>(currentInstruction);
    auto localIndex = localInstruction->getIndex();
    ValueType type;
    const auto& parameters = currentSignature->getParams();

    if (localIndex < parameters.size()) {
        type = parameters[localIndex]->getType();
    } else {
        type = currentCodeEntry->getLocals()[localIndex - parameters.size()]->getType();
    }

    switch(uint32_t(currentInstruction->getOpcode())) {
        case Opcode::local__get:
            pushOperand(type);

            break;

        case Opcode::local__set:
            popOperand(type);

            break;

        case Opcode::local__tee:
            popOperand(type);
            pushOperand(type);

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in checkSpecial.\n";
    }
}

void Validator::checkGlobal()
{
    auto* globalInstruction = static_cast<InstructionLocalIdx*>(currentInstruction);
    auto globalIndex = globalInstruction->getIndex();
    auto type = module->getGlobal(globalIndex)->getType();

    switch(uint32_t(currentInstruction->getOpcode())) {
        case Opcode::global__get:
            pushOperand(type);

            break;

        case Opcode::global__set:
            popOperand(type);

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in checkSpecial.\n";
    }
}

void Validator::checkSpecial()
{
    if (currentInstruction->getImmediateType() == ImmediateType::block) {
        checkBlock();
        return;
    }

    if (currentInstruction->getImmediateType() == ImmediateType::localIdx) {
        checkLocal();
        return;
    }

    if (currentInstruction->getImmediateType() == ImmediateType::globalIdx) {
        checkGlobal();
        return;
    }

    switch(uint32_t(currentInstruction->getOpcode())) {
        case Opcode::drop:
            popOperand();
            break;

        case Opcode::select:
            {
                popOperand(ValueType::i32);
                auto t1 = popOperand();
                auto t2 = popOperand(t1);
                pushOperand(t2);
            }

            break;

        case Opcode::unreachable:
            unreachable();

            break;

        case Opcode::end:
            pushOperands(popFrame());

            break;

        case Opcode::else_:
            {
                auto results = popFrame();

                pushFrame(results, results);
            }

            break;

        case Opcode::br:
            checkBr();

            break;

        case Opcode::br_if:
            checkBrIf();

            break;

        case Opcode::br_table:
            checkBrTable();

            break;

        case Opcode::call:
            checkCall();

            break;

        case Opcode::call_indirect:
            checkCallIndirect();

            break;

        case Opcode::return_:
            popOperands(frames.back().labelTypes);
            unreachable();

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in checkSpecial.\n";
            break;
    }
}

void Validator::check(Instruction* instruction)
{
    currentInstruction = instruction;

    auto opcode = currentInstruction->getOpcode();
    auto* instructionInfo = opcode.getInfo();

    switch(instructionInfo->signatureCode) {
        case SignatureCode::void_:
            break;

        case SignatureCode::f32_:
            pushOperand(ValueType::f32);
            break;

        case SignatureCode::f32__f32:
            popOperand(ValueType::f32);

            pushOperand(ValueType::f32);
            break;

        case SignatureCode::f32__f32_f32:
            popOperands(ValueType::f32, ValueType::f32);

            pushOperand(ValueType::f32);

            break;

        case SignatureCode::f32__f64:
            popOperand(ValueType::f64);

            pushOperand(ValueType::f32);
            break;

        case SignatureCode::f32__i32:
            popOperand(ValueType::i32);

            pushOperand(ValueType::f32);
            break;

        case SignatureCode::f32__i64:
            popOperand(ValueType::i64);

            pushOperand(ValueType::f32);
            break;

        case SignatureCode::f32__v128:
            popOperand(ValueType::v128);

            pushOperand(ValueType::f32);
            break;

        case SignatureCode::f64_:
            pushOperand(ValueType::f64);
            break;

        case SignatureCode::f64__f32:
            popOperand(ValueType::f32);

            pushOperand(ValueType::f64);
            break;

        case SignatureCode::f64__f64:
            popOperand(ValueType::f64);

            pushOperand(ValueType::f64);
            break;

        case SignatureCode::f64__f64_f64:
            popOperands(ValueType::f64, ValueType::f64);

            pushOperand(ValueType::f64);
            break;

        case SignatureCode::f64__i32:
            popOperand(ValueType::i32);

            pushOperand(ValueType::f64);
            break;

        case SignatureCode::f64__i64:
            popOperand(ValueType::i64);

            pushOperand(ValueType::f64);
            break;

        case SignatureCode::f64__v128:
            popOperand(ValueType::v128);

            pushOperand(ValueType::f64);
            break;

        case SignatureCode::i32_:
            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__f32:
            popOperand(ValueType::f32);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__f32_f32:
            popOperands(ValueType::f32, ValueType::f32);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__f64:
            popOperand(ValueType::f64);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__f64_f64:
            popOperands(ValueType::f64, ValueType::f64);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__i32:
            popOperand(ValueType::i32);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32:
            popOperands(ValueType::i32, ValueType::i32);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32_i32:
            popOperands(ValueType::i32, ValueType::i32, ValueType::i32);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32_i64:
            popOperands(ValueType::i64, ValueType::i32, ValueType::i32);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i64_i64:
            popOperands(ValueType::i64, ValueType::i64, ValueType::i32);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__i64:
            popOperand(ValueType::i64);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__i64_i64:
            popOperands(ValueType::i64, ValueType::i64);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i32__v128:
            popOperand(ValueType::v128);

            pushOperand(ValueType::i32);
            break;

        case SignatureCode::i64_:
            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__f32:
            popOperand(ValueType::f32);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__f64:
            popOperand(ValueType::f64);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__i32:
            popOperand(ValueType::i32);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__i32_i64:
            popOperands(ValueType::i64, ValueType::i32);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__i32_i64_i64:
            popOperands(ValueType::i64, ValueType::i64, ValueType::i32);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__i64:
            popOperand(ValueType::i64);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__i64_i64:
            popOperands(ValueType::i64, ValueType::i64);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::i64__v128:
            popOperand(ValueType::v128);

            pushOperand(ValueType::i64);
            break;

        case SignatureCode::v128_:
            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__f32:
            popOperand(ValueType::f32);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__f64:
            popOperand(ValueType::f64);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__i32:
            popOperand(ValueType::i32);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__i64:
            popOperand(ValueType::i64);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__v128:
            popOperand(ValueType::v128);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__v128_f32:
            popOperands(ValueType::f32, ValueType::v128);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__v128_f64:
            popOperands(ValueType::f64, ValueType::v128);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__v128_i32:
            popOperands(ValueType::i32, ValueType::v128);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__v128_i64:
            popOperands(ValueType::i64, ValueType::v128);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__v128_v128:
            popOperands(ValueType::v128, ValueType::v128);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::v128__v128_v128_v128:
            popOperands(ValueType::v128, ValueType::v128, ValueType::v128);

            pushOperand(ValueType::v128);
            break;

        case SignatureCode::void__i32_f32:
            popOperands(ValueType::f32, ValueType::i32);
            break;

        case SignatureCode::void__i32_f64:
            popOperands(ValueType::f64, ValueType::i32);
            break;

        case SignatureCode::void__i32_i32:
            popOperands(ValueType::i32, ValueType::i32);
            break;

        case SignatureCode::void__i32_i32_i32:
            popOperands(ValueType::i32, ValueType::i32, ValueType::i32);
            break;

        case SignatureCode::void__i32_i64:
            popOperands(ValueType::i64, ValueType::i32);
            break;

        case SignatureCode::void__i32_v128:
            popOperands(ValueType::v128, ValueType::i32);
            break;

        case SignatureCode::special:
            checkSpecial();
            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in check.\n";
    }
}

};

