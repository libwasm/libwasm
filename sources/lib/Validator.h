// Validator.h

#ifndef STACKCHECKER_H
#define STACKCHECKER_H

#include "Encodings.h"

#include <memory>
#include <vector>

namespace libwasm
{

class CheckContext;
class CheckErrorHandler;
class CodeEntry;
class Instruction;
class Local;
class Module;
class Signature;

void validate(CheckContext& context);

class Validator
{
    public:
        struct Frame
        {
            Frame(const std::vector<ValueType>& labelTypes,
                    const std::vector<ValueType>& endTypes);

            std::vector<ValueType> labelTypes;
            std::vector<ValueType> endTypes;
            size_t height = 0;
            bool unreachable = false;
        };

        Validator(CheckContext& c);

        void check(CodeEntry* code);

    private:
        void reset();

        void check(Instruction* instruction);
        bool underflow();
        void checkSpecial();
        void checkTable();
        void checkBlock();
        void checkLocal();
        void checkGlobal();
        void checkBr();
        void checkBrIf();
        void checkBrTable();
        void checkCall();
        void checkCallIndirect();
        void checkReturnCall();
        void checkReturnCallIndirect();

        void pushOperand(ValueType type);
        ValueType popOperand();
        ValueType popOperand(ValueType expect);
        void pushOperands(const std::vector<ValueType>& types);
        void popOperands(const std::vector<ValueType>& types);
        void popOperands(const std::vector<std::unique_ptr<Local>>& locals);

        void peekOperand(ValueType expect, size_t index);

        template<typename... Ts>
        void popOperands(Ts... types)
        {
            auto index = 0;

            (peekOperand(types, index++), ...);
            for (size_t i = 0; i < sizeof...(types); ++i) {
                popOperand();
            }
        }

        void pushFrame(const std::vector<ValueType>& labelTypes,
                const std::vector<ValueType>& endTypes);
        std::vector<ValueType> popFrame();
        void unreachable();
        const Frame& getFrame(size_t index);

        Instruction* currentInstruction = nullptr;
        CodeEntry* currentCodeEntry = nullptr;
        Signature* currentSignature = nullptr;
        
        CheckContext& context;
        Module* module;
        CheckErrorHandler& msgs;

        std::vector<ValueType> operands;
        std::vector<Frame> frames;
};

};
#endif
