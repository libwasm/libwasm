// ExpressionS.h

#ifndef SEXPRESSION_H
#define SEXPRESSION_H

#include "Encodings.h"

#include <iostream>
#include <memory>
#include <vector>

namespace libwasm
{

class CodeEntry;
class Instruction;
class InstructionContext;
class Local;
class Module;
class Signature;

struct MetaInstruction
{
    MetaInstruction(Instruction* instruction)
      : instruction(instruction)
    {
    }

    template<typename... Ts>
    void addOperands(Ts... types)
    {
        (operands.push_back(types), ...);
    }

    template<typename... Ts>
    void addResults(Ts... types)
    {
        (results.push_back(types), ...);
    }

    void addOperands(const std::vector<ValueType>& types);
    void addOperands(const std::vector<std::unique_ptr<Local>>& locals);

    void addOperand(ValueType type)
    {
        operands.push_back(type);
    }

    void addResult(ValueType type)
    {
        results.push_back(type);
    }

    void addResults(const std::vector<ValueType>& types);

    Instruction* instruction = nullptr;
    bool barrier = false;
    std::vector<ValueType> operands;
    std::vector<ValueType> results;
};

class ExpressionS
{
    public:
        ExpressionS(MetaInstruction* meta);
        ExpressionS(ExpressionS&& other) = default;

        void generate(std::ostream& os, InstructionContext& context, bool inBlock = false);

        MetaInstruction* meta;
        std::vector<ExpressionS> expressionSs;
        size_t size = 1;
        bool isEnd = false;
        bool isBlock = false;
        bool isIf = false;
        bool isThen = false;
        bool barrier = false;
};

class ExpressionSBuilder
{
    public:
        ExpressionSBuilder(Module* module);

        void generate(std::ostream& os, CodeEntry* code);

        void clear();
    private:
        bool checkOperand(std::vector<ExpressionS>& result, ValueType type, size_t index);

        bool checkOperands(std::vector<ExpressionS>& result, std::vector<ValueType>& types);

        template<typename... Ts>
        bool checkOperands(std::vector<ExpressionS>& result, Ts... types)
        {
            size_t index = 0;

            return (checkOperand(result, types, index++) && ...);
        }

        void addMeta();
        void addMeta(Instruction* instruction);
        void addSpecial(MetaInstruction* meta);
        void addCall(MetaInstruction* meta);
        void addCallIndirect(MetaInstruction* meta);
        void addGlobal(MetaInstruction* meta);
        void addLocal(MetaInstruction* meta);

        void buildExpressionSs(std::vector<ExpressionS>& result, bool inBlock = false);
        void generateExpressionSs(std::ostream& os, std::vector<ExpressionS>& result);

        Instruction* currentInstruction = nullptr;
        CodeEntry* currentCodeEntry = nullptr;
        Signature* currentSignature = nullptr;
        size_t currentMetaIndex = 0;

        Module* module = nullptr;

        std::vector<MetaInstruction> metaInstructions;
        std::vector<ExpressionS> expressionSs;
};

};

#endif
