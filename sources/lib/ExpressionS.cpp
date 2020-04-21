// S_expression.cpp

#include "ExpressionS.h"

#include "BackBone.h"
#include "Instruction.h"
#include "Module.h"

namespace libwasm
{

void MetaInstruction::addOperands(const std::vector<ValueType>& types)
{
    for (auto i = types.size(); i-- > 0; ) {
        addOperand(types[i]);
    }
}

void MetaInstruction::addResults(const std::vector<ValueType>& types)
{
    for (auto i = types.size(); i-- > 0; ) {
        addResult(types[i]);
    }
}

void MetaInstruction::addOperands(const std::vector<std::unique_ptr<Local>>& locals)
{
    for (auto i = locals.size(); i-- > 0; ) {
        addOperand(locals[i]->getType());
    }
}

ExpressionS::ExpressionS(MetaInstruction* meta)
  : meta(meta), barrier(meta->barrier)
{
}

void ExpressionS::generate(std::ostream& os, InstructionContext& context, bool inBlock)
{
    if (isEnd) {
        context.leaveBlock();
        return;
    }

    if (inBlock || isBlock || isIf) {
        os << "\n " << context.getIndent();
    }

    os << " (";

    if (isThen) {
        os << "then";
    } else {
        meta->instruction->generate(os, context);
    }

    for (auto& expressionS : expressionSs) {
        expressionS.generate(os, context, isBlock || isIf);
    }

    os << ')';
}

void ExpressionSBuilder::addLocal()
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

    switch(currentInstruction->getOpcode()) {
        case Opcode::local__get:
            currentMeta->addResult(type);

            break;

        case Opcode::local__set:
            currentMeta->addOperand(type);

            break;

        case Opcode::local__tee:
            currentMeta->addOperand(type);
            currentMeta->addResult(type);

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in addLocal.\n";
    }
}

void ExpressionSBuilder::addGlobal()
{
    auto* globalInstruction = static_cast<InstructionGlobalIdx*>(currentInstruction);
    auto globalIndex = globalInstruction->getIndex();
    auto type = module->getGlobal(globalIndex)->getType();

    switch(currentInstruction->getOpcode()) {
        case Opcode::global__get:
            currentMeta->addResult(type);

            break;

        case Opcode::global__set:
            currentMeta->addOperand(type);

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in addGlobal.\n";
    }
}

void ExpressionSBuilder::addSpecial()
{
    if (currentInstruction->getImmediateType() == ImmediateType::localIdx) {
        addLocal();
        return;
    }

    if (currentInstruction->getImmediateType() == ImmediateType::globalIdx) {
        addGlobal();
        return;
    }

    switch(currentInstruction->getOpcode()) {
        case Opcode::drop:
            currentMeta->addOperand(ValueType::any);
            break;

        case Opcode::ref__func:
            currentMeta->addResult(ValueType::funcref);
            break;

        case Opcode::ref__null:
            currentMeta->addResult(ValueType::nullref);
            break;

        case Opcode::ref__is_null:
            currentMeta->addOperand(ValueType::anyref);

            currentMeta->addResult(ValueType::i32);
            break;

        case Opcode::br_if:
            currentMeta->addOperand(ValueType::i32);

            break;

        case Opcode::call:
            addCall();

            break;

        case Opcode::call_indirect:
            addCallIndirect();

            break;

        default:
            currentMeta->barrier = true;

            break;
    }
}

void ExpressionSBuilder::addCall()
{
    auto* callInstruction = static_cast<InstructionFunctionIdx*>(currentInstruction);
    auto* signature = module->getFunction(callInstruction->getIndex())->getSignature();

    currentMeta->addOperands(signature->getParams());
    currentMeta->addResults(signature->getResults());
}

void ExpressionSBuilder::addCallIndirect()
{
    auto& types = module->getTypeSection()->getTypes();
    auto* indirectInstruction = static_cast<InstructionIndirect*>(currentInstruction);
    size_t typeIndex = indirectInstruction->getIndex();
    auto* signature = types[typeIndex]->getSignature();

    currentMeta->addOperand(ValueType::i32);

    currentMeta->addOperands(signature->getParams());
    currentMeta->addResults(signature->getResults());
}

void ExpressionSBuilder::addMeta(Instruction* instruction)
{
    auto opcode = instruction->getOpcode();
    auto* instructionInfo = opcode.getInfo();

    currentInstruction = instruction;
    metaInstructions.emplace_back(instruction);

    currentMeta = &metaInstructions.back();

    switch(instructionInfo->signatureCode) {
        case SignatureCode::void_:
            break;

        case SignatureCode::f32_:
            currentMeta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__f32:
            currentMeta->addOperand(ValueType::f32);

            currentMeta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__f32_f32:
            currentMeta->addOperands(ValueType::f32, ValueType::f32);

            currentMeta->addResult(ValueType::f32);

            break;

        case SignatureCode::f32__f64:
            currentMeta->addOperand(ValueType::f64);

            currentMeta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__i32:
            currentMeta->addOperand(ValueType::i32);

            currentMeta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__i64:
            currentMeta->addOperand(ValueType::i64);

            currentMeta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__v128:
            currentMeta->addOperand(ValueType::v128);

            currentMeta->addResult(ValueType::f32);
            break;

        case SignatureCode::f64_:
            currentMeta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__f32:
            currentMeta->addOperand(ValueType::f32);

            currentMeta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__f64:
            currentMeta->addOperand(ValueType::f64);

            currentMeta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__f64_f64:
            currentMeta->addOperands(ValueType::f64, ValueType::f64);

            currentMeta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__i32:
            currentMeta->addOperand(ValueType::i32);

            currentMeta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__i64:
            currentMeta->addOperand(ValueType::i64);

            currentMeta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__v128:
            currentMeta->addOperand(ValueType::v128);

            currentMeta->addResult(ValueType::f64);
            break;

        case SignatureCode::i32_:
            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f32:
            currentMeta->addOperand(ValueType::f32);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f32_f32:
            currentMeta->addOperands(ValueType::f32, ValueType::f32);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f64:
            currentMeta->addOperand(ValueType::f64);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f64_f64:
            currentMeta->addOperands(ValueType::f64, ValueType::f64);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32:
            currentMeta->addOperand(ValueType::i32);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32:
            currentMeta->addOperands(ValueType::i32, ValueType::i32);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32_i32:
            currentMeta->addOperands(ValueType::i32, ValueType::i32, ValueType::i32);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::i32, ValueType::i32);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i64_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::i64, ValueType::i32);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i64:
            currentMeta->addOperand(ValueType::i64);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i64_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::i64);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__v128:
            currentMeta->addOperand(ValueType::v128);

            currentMeta->addResult(ValueType::i32);
            break;

        case SignatureCode::i64_:
            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__f32:
            currentMeta->addOperand(ValueType::f32);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__f64:
            currentMeta->addOperand(ValueType::f64);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i32:
            currentMeta->addOperand(ValueType::i32);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i32_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::i32);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i32_i64_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::i64, ValueType::i32);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i64:
            currentMeta->addOperand(ValueType::i64);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i64_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::i64);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__v128:
            currentMeta->addOperand(ValueType::v128);

            currentMeta->addResult(ValueType::i64);
            break;

        case SignatureCode::v128_:
            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__f32:
            currentMeta->addOperand(ValueType::f32);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__f64:
            currentMeta->addOperand(ValueType::f64);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__i32:
            currentMeta->addOperand(ValueType::i32);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__i64:
            currentMeta->addOperand(ValueType::i64);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128:
            currentMeta->addOperand(ValueType::v128);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_f32:
            currentMeta->addOperands(ValueType::f32, ValueType::v128);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_f64:
            currentMeta->addOperands(ValueType::f64, ValueType::v128);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_i32:
            currentMeta->addOperands(ValueType::i32, ValueType::v128);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::v128);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_v128:
            currentMeta->addOperands(ValueType::v128, ValueType::v128);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_v128_v128:
            currentMeta->addOperands(ValueType::v128, ValueType::v128, ValueType::v128);

            currentMeta->addResult(ValueType::v128);
            break;

        case SignatureCode::void__i32:
            currentMeta->addOperand(ValueType::f32);
            break;

        case SignatureCode::void__i32_f32:
            currentMeta->addOperands(ValueType::f32, ValueType::i32);
            break;

        case SignatureCode::void__i32_f64:
            currentMeta->addOperands(ValueType::f64, ValueType::i32);
            break;

        case SignatureCode::void__i32_i32:
            currentMeta->addOperands(ValueType::i32, ValueType::i32);
            break;

        case SignatureCode::void__i32_i32_i32:
            currentMeta->addOperands(ValueType::i32, ValueType::i32, ValueType::i32);
            break;

        case SignatureCode::void__i32_i64:
            currentMeta->addOperands(ValueType::i64, ValueType::i32);
            break;

        case SignatureCode::void__i32_v128:
            currentMeta->addOperands(ValueType::v128, ValueType::i32);
            break;

        case SignatureCode::special:
            addSpecial();
            break;

        default:
            std::cerr << "Unimplemented opcode '" << instruction->getOpcode() << " in addMeta.\n";
    }
}

void ExpressionSBuilder::buildExpressionSs(std::vector<ExpressionS>& result, bool inBlock)
{
    while (currentMetaIndex < metaInstructions.size()) {
        auto& metaInstruction = metaInstructions[currentMetaIndex];
        auto opcode = metaInstruction.instruction->getOpcode();

        if (inBlock) {
            if (opcode == Opcode::end || opcode == Opcode::else_) {
                return;
            }
        }

        currentMetaIndex++;

        ExpressionS expressionS(&metaInstruction);

        if (opcode == Opcode::end) {
            expressionS.isEnd = true;
        }

        if (checkOperands(result, metaInstruction.operands)) {
            for (auto i = metaInstruction.operands.size(); i-- > 0;) {
                expressionS.expressionSs.push_back(std::move(result[result.size() - i - 1]));
            }

            for (auto i = metaInstruction.operands.size(); i-- > 0;) {
                result.pop_back();
            }
        }

        if (metaInstruction.instruction->getImmediateType() == ImmediateType::block) {
            if (opcode == Opcode::if_) {
                expressionS.isIf = true;
                expressionS.expressionSs.emplace_back(nullptr);
                expressionS.expressionSs.back().isThen = true;
                buildExpressionSs(expressionS.expressionSs.back().expressionSs, true);

                auto& nextMeta = metaInstructions[currentMetaIndex];
                auto nextOpcode = nextMeta.instruction->getOpcode();

                if (nextOpcode == Opcode::else_) {
                    currentMetaIndex++;
                    expressionS.expressionSs.emplace_back(&nextMeta);
                    expressionS.expressionSs.back().isBlock = true;
                    buildExpressionSs(expressionS.expressionSs.back().expressionSs, true);
                }
            } else {
                expressionS.isBlock = true;
                buildExpressionSs(expressionS.expressionSs, true);
            }

            auto& nextMeta = metaInstructions[currentMetaIndex];
            auto nextOpcode = nextMeta.instruction->getOpcode();

            if (nextOpcode == Opcode::end) {
                expressionS.expressionSs.back().expressionSs.emplace_back(&nextMeta);
                expressionS.expressionSs.back().expressionSs.back().isEnd = true;
                currentMetaIndex++;
            }
        }

        for (auto& expression : expressionS.expressionSs) {
            expressionS.size += expression.size;
        }

        result.push_back(std::move(expressionS));
    }
}

void ExpressionSBuilder::generateExpressionSs(std::ostream& os, std::vector<ExpressionS>& result)
{
    InstructionContext context;

    context.enterBlock();

    for (auto& expressionS : result) {
        expressionS.generate(os, context, true);
    }
}

void ExpressionSBuilder::addMeta()
{
    for (auto& instruction : currentCodeEntry->getExpression()->getInstructions()) {
        addMeta(instruction.get());
    }
}

void ExpressionSBuilder::generate(std::ostream& os, CodeEntry* code)
{
    clear();
    currentCodeEntry = code;
    currentSignature = module->getFunction(code->getNumber())->getSignature();

    addMeta();
    buildExpressionSs(expressionSs);
    generateExpressionSs(os, expressionSs);
}

bool ExpressionSBuilder::checkOperand(std::vector<ExpressionS>& result, ValueType type, size_t index)
{
    auto size = result.size();

    if (index >= size) {
        return false;
    }

    auto& meta = result[size - index - 1].meta;

    if (meta->results.empty()) {
        return false;
    }

    return meta->results[0] == type;
}

bool ExpressionSBuilder::checkOperands(std::vector<ExpressionS>& result, std::vector<ValueType>& types)
{
    auto size = types.size();

    for (size_t i = 0; i < size; ++i) {
        if (!checkOperand(result, types[i], i)) {
            return false;
        }
    }

    return true;
}


ExpressionSBuilder::ExpressionSBuilder(Module* module)
  : module(module)
{
}

void ExpressionSBuilder::clear()
{
    currentInstruction = nullptr;
    currentMeta = nullptr;
    currentCodeEntry = nullptr;
    currentSignature = nullptr;
    currentMetaIndex = 0;

    metaInstructions.clear();
    expressionSs.clear();

}

};


