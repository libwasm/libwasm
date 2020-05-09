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
  : meta(meta)
{
    if (meta != nullptr) {
        barrier = meta->barrier;
    }
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

    if (size > 7) {
        if (!(isBlock || isIf)) {
            context.enter();
        }

        for (auto& expressionS : expressionSs) {
            expressionS.generate(os, context, true);
        }

        if (!(isBlock || isIf)) {
            context.leave();
        }
    } else {
        for (auto& expressionS : expressionSs) {
            expressionS.generate(os, context);
        }
    }

    os << ')';
}

void ExpressionSBuilder::addLocal(MetaInstruction* meta)
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
            meta->addResult(type);

            break;

        case Opcode::local__set:
            meta->addOperand(type);

            break;

        case Opcode::local__tee:
            meta->addOperand(type);
            meta->addResult(type);

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in addLocal.\n";
    }
}

void ExpressionSBuilder::addGlobal(MetaInstruction* meta)
{
    auto* globalInstruction = static_cast<InstructionGlobalIdx*>(currentInstruction);
    auto globalIndex = globalInstruction->getIndex();
    auto type = module->getGlobal(globalIndex)->getType();

    switch(currentInstruction->getOpcode()) {
        case Opcode::global__get:
            meta->addResult(type);

            break;

        case Opcode::global__set:
            meta->addOperand(type);

            break;

        default:
            std::cerr << "Unimplemented opcode '" << currentInstruction->getOpcode() << " in addGlobal.\n";
    }
}

void ExpressionSBuilder::addSpecial(MetaInstruction* meta)
{
    if (currentInstruction->getImmediateType() == ImmediateType::localIdx) {
        addLocal(meta);
        return;
    }

    if (currentInstruction->getImmediateType() == ImmediateType::globalIdx) {
        addGlobal(meta);
        return;
    }

    switch(currentInstruction->getOpcode()) {
        case Opcode::ref__func:
            meta->addResult(ValueType::funcref);
            break;

        case Opcode::ref__null:
            meta->addResult(ValueType::nullref);
            break;

        case Opcode::ref__is_null:
            meta->addOperand(ValueType::anyref);

            meta->addResult(ValueType::i32);
            break;

        case Opcode::br_if:
        case Opcode::br_table:
        case Opcode::if_:
            meta->addOperand(ValueType::i32);

            break;

        case Opcode::call:
            addCall(meta);

            break;

        case Opcode::call_indirect:
            addCallIndirect(meta);

            break;

        case Opcode::return_:
            meta->addOperands(currentSignature->getResults());

            break;
        default:
            meta->barrier = true;

            break;
    }
}

void ExpressionSBuilder::addCall(MetaInstruction* meta)
{
    auto* callInstruction = static_cast<InstructionFunctionIdx*>(currentInstruction);
    auto* signature = module->getFunction(callInstruction->getIndex())->getSignature();

    meta->addOperands(signature->getParams());
    meta->addResults(signature->getResults());
}

void ExpressionSBuilder::addCallIndirect(MetaInstruction* meta)
{
    auto* indirectInstruction = static_cast<InstructionIndirect*>(currentInstruction);
    auto typeIndex = indirectInstruction->getTypeIndex();
    auto* signature = module->getType(typeIndex)->getSignature();

    meta->addOperand(ValueType::i32);

    meta->addOperands(signature->getParams());
    meta->addResults(signature->getResults());
}

void ExpressionSBuilder::addMeta(Instruction* instruction)
{
    auto opcode = instruction->getOpcode();
    auto* instructionInfo = opcode.getInfo();

    currentInstruction = instruction;
    metaInstructions.emplace_back(instruction);

    auto* meta = &metaInstructions.back();

    switch(instructionInfo->signatureCode) {
        case SignatureCode::void_:
            break;

        case SignatureCode::f32_:
            meta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__f32:
            meta->addOperand(ValueType::f32);

            meta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__f32_f32:
            meta->addOperands(ValueType::f32, ValueType::f32);

            meta->addResult(ValueType::f32);

            break;

        case SignatureCode::f32__f64:
            meta->addOperand(ValueType::f64);

            meta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__i32:
            meta->addOperand(ValueType::i32);

            meta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__i64:
            meta->addOperand(ValueType::i64);

            meta->addResult(ValueType::f32);
            break;

        case SignatureCode::f32__v128:
            meta->addOperand(ValueType::v128);

            meta->addResult(ValueType::f32);
            break;

        case SignatureCode::f64_:
            meta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__f32:
            meta->addOperand(ValueType::f32);

            meta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__f64:
            meta->addOperand(ValueType::f64);

            meta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__f64_f64:
            meta->addOperands(ValueType::f64, ValueType::f64);

            meta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__i32:
            meta->addOperand(ValueType::i32);

            meta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__i64:
            meta->addOperand(ValueType::i64);

            meta->addResult(ValueType::f64);
            break;

        case SignatureCode::f64__v128:
            meta->addOperand(ValueType::v128);

            meta->addResult(ValueType::f64);
            break;

        case SignatureCode::i32_:
            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f32:
            meta->addOperand(ValueType::f32);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f32_f32:
            meta->addOperands(ValueType::f32, ValueType::f32);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f64:
            meta->addOperand(ValueType::f64);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__f64_f64:
            meta->addOperands(ValueType::f64, ValueType::f64);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32:
            meta->addOperand(ValueType::i32);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32:
            meta->addOperands(ValueType::i32, ValueType::i32);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32_i32:
            meta->addOperands(ValueType::i32, ValueType::i32, ValueType::i32);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i32_i64:
            meta->addOperands(ValueType::i64, ValueType::i32, ValueType::i32);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i32_i64_i64:
            meta->addOperands(ValueType::i64, ValueType::i64, ValueType::i32);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i64:
            meta->addOperand(ValueType::i64);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__i64_i64:
            meta->addOperands(ValueType::i64, ValueType::i64);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i32__v128:
            meta->addOperand(ValueType::v128);

            meta->addResult(ValueType::i32);
            break;

        case SignatureCode::i64_:
            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__f32:
            meta->addOperand(ValueType::f32);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__f64:
            meta->addOperand(ValueType::f64);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i32:
            meta->addOperand(ValueType::i32);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i32_i64:
            meta->addOperands(ValueType::i64, ValueType::i32);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i32_i64_i64:
            meta->addOperands(ValueType::i64, ValueType::i64, ValueType::i32);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i64:
            meta->addOperand(ValueType::i64);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__i64_i64:
            meta->addOperands(ValueType::i64, ValueType::i64);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::i64__v128:
            meta->addOperand(ValueType::v128);

            meta->addResult(ValueType::i64);
            break;

        case SignatureCode::v128_:
            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__f32:
            meta->addOperand(ValueType::f32);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__f64:
            meta->addOperand(ValueType::f64);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__i32:
            meta->addOperand(ValueType::i32);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__i64:
            meta->addOperand(ValueType::i64);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128:
            meta->addOperand(ValueType::v128);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_f32:
            meta->addOperands(ValueType::f32, ValueType::v128);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_f64:
            meta->addOperands(ValueType::f64, ValueType::v128);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_i32:
            meta->addOperands(ValueType::i32, ValueType::v128);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_i64:
            meta->addOperands(ValueType::i64, ValueType::v128);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_v128:
            meta->addOperands(ValueType::v128, ValueType::v128);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::v128__v128_v128_v128:
            meta->addOperands(ValueType::v128, ValueType::v128, ValueType::v128);

            meta->addResult(ValueType::v128);
            break;

        case SignatureCode::void__i32:
            meta->addOperand(ValueType::f32);
            break;

        case SignatureCode::void__i32_f32:
            meta->addOperands(ValueType::f32, ValueType::i32);
            break;

        case SignatureCode::void__i32_f64:
            meta->addOperands(ValueType::f64, ValueType::i32);
            break;

        case SignatureCode::void__i32_i32:
            meta->addOperands(ValueType::i32, ValueType::i32);
            break;

        case SignatureCode::void__i32_i32_i32:
            meta->addOperands(ValueType::i32, ValueType::i32, ValueType::i32);
            break;

        case SignatureCode::void__i32_i64:
            meta->addOperands(ValueType::i64, ValueType::i32);
            break;

        case SignatureCode::void__i32_v128:
            meta->addOperands(ValueType::v128, ValueType::i32);
            break;

        case SignatureCode::special:
            addSpecial(meta);
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
        } else if (opcode  == Opcode::drop) {
            if (!result.empty()) {
                auto* meta = result.back().meta;

                if (!meta->results.empty()) {
                    expressionS.expressionSs.push_back(std::move(result.back()));
                    result.pop_back();
                }
            }
        } else if (opcode  == Opcode::select) {
            auto size = result.size();

            if (size >= 3) {
                if (checkOperand(result, ValueType::i32, 0)) {
                    if (auto* meta1 = result[size - 2].meta; !meta1->results.empty()) {
                        auto type1 = meta1->results[0] ;

                        if (auto* meta2 = result[size - 3].meta; !meta2->results.empty()) {
                            auto type2 = meta2->results[0] ;

                            if (type1 == type2) {
                                for (auto i = 3; i-- > 0;) {
                                    expressionS.expressionSs.push_back(std::move(result[result.size() - i - 1]));
                                }

                                for (auto i = 3; i-- > 0;) {
                                    result.pop_back();
                                }

                                expressionS.meta->results.push_back(type1);
                                expressionS.meta->addOperands(ValueType::i32, type1, type1);
                            }
                        }
                    }
                }
            }
        } else if (checkOperands(result, metaInstruction.operands)) {
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
                expressionS.expressionSs.emplace_back(&nextMeta);
                expressionS.expressionSs.back().isEnd = true;
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
    InstructionContext context(module, currentCodeEntry);

    context.setComments(false);
    context.enter();

    for (auto& expressionS : result) {
        expressionS.generate(os, context, true);
    }

    context.leave();
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

    InstructionContext context(module, currentCodeEntry);

    generateExpressionSs(os, expressionSs);
}

bool ExpressionSBuilder::checkOperand(std::vector<ExpressionS>& result, ValueType type, size_t index)
{
    auto size = result.size();

    if (index >= size) {
        return false;
    }

    auto* meta = result[size - index - 1].meta;

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
    currentCodeEntry = nullptr;
    currentSignature = nullptr;
    currentMetaIndex = 0;

    metaInstructions.clear();
    expressionSs.clear();

}

};


