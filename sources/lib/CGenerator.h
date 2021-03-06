// CGenerator.h

#ifndef CGENERATOR_H
#define CGENERATOR_H

#include "Encodings.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace libwasm
{

class CGenerator;

class CodeEntry;
class Instruction;
class InstructionBlock;
class InstructionContext;
class Local;
class Module;
class Signature;

class CIf;

class CNode
{
    public:
        enum CNodeKind
        {
            kNone,
            kBinauryExpression,
            kBranch,
            kBreak,
            kCall,
            kCallIndirect,
            kCast,
            kCompound,
            kContinue,
            kF32,
            kF64,
            kFunction,
            kI32,
            kI64,
            kIf,
            kLabel,
            kLoad,
            kLoop,
            kNameUse,
            kPostfixExpression,
            kReturn,
            kStore,
            kSubscript,
            kSwitch,
            kTernaryExpression,
            kUnaryExpression,
            kV128,
            kVariable,
        };

        CNode(CNodeKind kind)
          : nodeKind(kind)
        {
        }

        virtual ~CNode();

        template<typename T>
        T* castTo()
        {
            if (nodeKind == T::kind) {
                return static_cast<T*>(this);
            } else {
                return nullptr;
            }
        }

        void link(CNode* p);
        void link(CNode* parent, CNode* n);
        void linkFirst(CNode* p);

        void unlink();

        CNode* traverseToNext(CNode* root);

        virtual bool equals(CNode* other) = 0;

        virtual void generateC(std::ostream& os, CGenerator& generator)
        {
            std::cerr << "Not implemented kind " << nodeKind << std::endl;
        }

        auto getKind() const
        {
            return nodeKind;
        }

        auto* getParent() const
        {
            return parent;
        }

        auto* getNext() const
        {
            return next;
        }

        auto* getPrevious() const
        {
            return previous;
        }

        auto* getChild() const
        {
            return child;
        }

        auto* getLastChild() const
        {
            return lastChild;
        }

        CNode* findNext(CNodeKind kind);

        virtual bool hasSideEffects() const
        {
            return true;
        }

        void setNopped(bool value)
        {
            nopped = value;
        }

        auto isNopped() const
        {
            return nopped;
        }

        bool contains(CNode* node);

    protected:
        CNodeKind nodeKind = kNone;
        bool nopped = false;
        CNode* parent = nullptr;
        CNode* next = nullptr;
        CNode* previous = nullptr;
        CNode* child = nullptr;
        CNode* lastChild = nullptr;

        CNode(const CNode&) = delete;
        CNode& operator= (const CNode&) = delete;
};

class CCompound : public CNode
{
    public:
        static const CNodeKind kind = kCompound;

        CCompound()
            : CNode(kind)
        {
        }

        bool empty() const
        {
            return child == nullptr;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        void addStatement(CNode* statement);
        void enhance(CGenerator& generator);
        void enhanceVariables(CGenerator& generator);
        void enhanceIf(CIf* ifStatement, CGenerator& generator);
        void flatten();
};

class CLoop : public CNode
{
    public:
        static const CNodeKind kind = kLoop;

        CLoop()
            : CNode(kLoop), body(new CCompound)
        {
            body->link(this);
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        void addStatement(CNode* statement);

        auto* getBody() const
        {
            return body;
        }

        auto* getInitialize() const
        {
            return initialize;
        }

        auto* getCondition() const
        {
            return condition;
        }

        auto* getIncrement() const
        {
            return increment;
        }

        void setInitialize(CNode* node)
        {
            initialize = node;
            initialize->link(this);
        }

        void setCondition(CNode* node)
        {
            condition = node;
            condition->link(this);
        }

        void setIncrement(CNode* node)
        {
            increment = node;
            increment->link(this);
        }

        void enhance(CGenerator& generator);
        void enhanceContinues(unsigned loopLabel, CGenerator& generator);
        void enhanceBreaks(unsigned loopLabel, CGenerator& generator);
        void tryWhile2For(CGenerator& generator, CIf* ifParent);

    private:
        enum
        {
            isNone,
            isDo,
            isFor,
            isWhile
        } loopType = isNone;

        CCompound* body = nullptr;
        CNode* initialize = nullptr;
        CNode* condition = nullptr;
        CNode* increment = nullptr;
};

class CBreak : public CNode
{
    public:
        static const CNodeKind kind = kBreak;

        CBreak()
            : CNode(kind)
        {
        }

        virtual bool equals(CNode* other) override
        {
            return other->getKind() == kind;
        }

        virtual void generateC(std::ostream& os, CGenerator& generator) override;
};

class CContinue : public CNode
{
    public:
        static const CNodeKind kind = kContinue;

        CContinue()
            : CNode(kind)
        {
        }

        virtual bool equals(CNode* other) override
        {
            return other->getKind() == kind;
        }

        virtual void generateC(std::ostream& os, CGenerator& generator) override;
};

class CBranch : public CNode
{
    public:
        static const CNodeKind kind = kBranch;

        CBranch(unsigned label)
            : CNode(kind), label(label)
        {
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getLabel() const
        {
            return label;
        }

    private:
        unsigned label = 0;
};

class CSubscript : public CNode
{
    public:
        static const CNodeKind kind = kSubscript;

        CSubscript(CNode* array, CNode* subscript)
            : CNode(kind), array(array), subscript(subscript)
        {
            array->link(this);
            subscript->link(this);
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto* getArray() const
        {
            return array;
        }

        auto* getSubscript() const
        {
            return subscript;
        }

    private:
        CNode* array = nullptr;
        CNode* subscript = nullptr;
};

class CSwitch : public CNode
{
    public:
        static const CNodeKind kind = kSwitch;

        CSwitch(CNode* condition)
            : CNode(kind), condition(condition)
        {
            condition->link(this);
            defaultStatements = new CCompound;
            defaultStatements->link(this);
        }

        ~CSwitch()
        {
            for (auto* cs : cases) {
                delete(cs);
            }
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        struct Case
        {
            Case(uint64_t value)
              : value(value)
            {
                statements = new CCompound;
            }

            ~Case()
            {
                delete statements;
            }

            void addStatement(CNode* statement);

            uint64_t value = 0;
            CCompound* statements = nullptr;
        };

        auto* getCondition() const
        {
            return condition;
        }

        auto& getCases()
        {
            return cases;
        }

        auto* getDefault() const
        {
            return defaultStatements;
        }

        void addCase(uint64_t value, CNode* statement);
        void addDefault(CNode* statement);

    private:
        CNode* condition = nullptr;
        std::vector<Case*> cases;
        CCompound* defaultStatements = nullptr;
};

class CBinaryExpression : public CNode
{
    public:
        static const CNodeKind kind = kBinauryExpression;

        CBinaryExpression(std::string_view op, CNode* left, CNode* right)
            : CNode(kind), left(left), right(right), op(op)
        {
            left->link(this);
            right->link(this);
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        void setOp(std::string_view o)
        {
            op = o;
        }

        std::string_view getOp() const
        {
            return op;
        }

        auto* getLeft() const
        {
            return left;
        }

        auto* getRight() const
        {
            return right;
        }

        virtual bool hasSideEffects() const override
        {
            return left->hasSideEffects() || right->hasSideEffects();
        }

        void enhanceAssignment();
        void enhance();

    private:
        CNode* left = nullptr;
        CNode* right = nullptr;
        std::string op;
};

class CCallIndirect : public CNode
{
    public:
        static const CNodeKind kind = kCallIndirect;

        CCallIndirect(uint32_t typeIndex, uint32_t tableIndex, CNode* indexInTable)
            : CNode(kind), typeIndex(typeIndex), tableIndex(tableIndex),
              indexInTable(indexInTable)
        {
            indexInTable->link(this);
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        void addArgument(CNode* argument);
        void reverseArguments();

        auto getTypeIndex() const
        {
            return typeIndex;
        }

        auto getTableIndex() const
        {
            return tableIndex;
        }

        auto* getIndexInTable() const
        {
            return indexInTable;
        }

        auto& getArguments()
        {
            return arguments;
        }

    private:
        uint32_t typeIndex = 0;
        uint32_t tableIndex = 0;
        CNode* indexInTable = nullptr;
        std::vector<CNode*> arguments;
};

class CCall : public CNode
{
    public:
        static const CNodeKind kind = kCall;

        CCall(std::string_view name)
            : CNode(kind), functionName(name)
        {
        }

        auto getPure() const
        {
            return pure;
        }

        void setPure(bool p)
        {
            pure = p;
        }

        virtual bool hasSideEffects() const override
        {
            return !pure;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        std::string_view getFunctionName() const
        {
            return functionName;
        }

        auto& getArguments()
        {
            return arguments;
        }

        void addArgument(CNode* argument);
        void reverseArguments();

    private:
        bool pure = false;
        std::string functionName;
        std::vector<CNode*> arguments;
};

class CCast : public CNode
{
    public:
        static const CNodeKind kind = kCast;

        CCast(std::string_view type, CNode* operand)
            : CNode(kind), type(type), operand(operand)
        {
            operand->link(this);
        }

        virtual bool hasSideEffects() const override
        {
            return operand->hasSideEffects();
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getType()
        {
            return type;
        }

        auto* getOperand() const
        {
            return operand;
        }

    private:
        std::string_view type;
        CNode* operand = nullptr;
};

class CLabel : public CNode
{
    public:
        static const CNodeKind kind = kLabel;

        CLabel(unsigned label)
            : CNode(kind), label(label)
        {
        }

        auto getLabel() const
        {
            return label;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

    private:
        unsigned label = 0;
};

class CVariable : public CNode
{
    public:
        static const CNodeKind kind = kVariable;

        CVariable(ValueType type, std::string_view name, CNode* initialValue = nullptr)
            : CNode(kind), type(type), name(name), initialValue(initialValue)
        {
            if (initialValue) {
                initialValue->link(this);
            }
        }

        std::string_view getName() const
        {
            return name;
        }

        auto getType() const
        {
            return type;
        }

        void setInitialValue(CNode* v)
        {
            assert(initialValue == nullptr);

            initialValue = v;
            v->link(this);
        }

        auto* getInitialValue() const
        {
            return initialValue;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

    private:
        ValueType type;
        std::string name;
        CNode* initialValue = nullptr;
};

class CFunction : public CNode
{
    public:
        static const CNodeKind kind = kFunction;

        CFunction(Signature* signature)
            : CNode(kind), signature(signature)
        {
            statements = new CCompound;
            statements->link(this);
        }

        virtual bool equals(CNode* other) override
        {
            return false;
        }

        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        void addStatement(CNode* statement);

        auto* getStatements() const
        {
            return statements;
        }

        auto* getSignature() const
        {
            return signature;
        }

        void enhance(CGenerator& generator);

    private:
        std::string name;
        Signature* signature = nullptr;
        CCompound* statements = nullptr;
};

class CI32 : public CNode
{
    public:
        static const CNodeKind kind = kI32;

        CI32(uint32_t value)
           : CNode(kind), value(value)
        {
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getValue() const
        {
            return value;
        }

        void setValue(int32_t v)
        {
            value = v;
        }

        virtual bool hasSideEffects() const override
        {
            return false;
        }

    private:
        int32_t value = 0;
};

class CI64 : public CNode
{
    public:
        static const CNodeKind kind = kI64;

        CI64(uint64_t value)
           : CNode(kind), value(value)
        {
        }

        void setValue(int64_t v)
        {
            value = v;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getValue() const
        {
            return value;
        }

        virtual bool hasSideEffects() const override
        {
            return false;
        }

    private:
        int64_t value = 0;
};

class CF32 : public CNode
{
    public:
        static const CNodeKind kind = kF32;

        CF32(float value)
           : CNode(kind), value(value)
        {
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getValue() const
        {
            return value;
        }

        void setValue(float v)
        {
            value = v;
        }

        virtual bool hasSideEffects() const override
        {
            return false;
        }

    private:
        float value = 0;
};

class CF64 : public CNode
{
    public:
        static const CNodeKind kind = kF64;

        CF64(double value)
           : CNode(kind), value(value)
        {
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getValue() const
        {
            return value;
        }

        void setValue(double v)
        {
            value = v;
        }

        virtual bool hasSideEffects() const override
        {
            return false;
        }

    private:
        double value = 0;
};

class CV128 : public CNode
{
    public:
        static const CNodeKind kind = kV128;

        CV128(v128_t value)
           : CNode(kind), value(value)
        {
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getValue() const
        {
            return value;
        }

        virtual bool hasSideEffects() const override
        {
            return false;
        }

    private:
        v128_t value = { 0 };
};

class CIf : public CNode
{
    public:
        static const CNodeKind kind = kIf;

        CIf(CNode* condition, unsigned label = 0, std::vector<ValueType> types = {})
            : CNode(kind), condition(condition), label(label), types(std::move(types))
        {
            condition->link(this);
            tempDeclarations = new CCompound;
            tempDeclarations->link(this);
            thenStatements = new CCompound;
            thenStatements->link(this);
            elseStatements = new CCompound;
            elseStatements->link(this);
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        void setResultDeclaration(CNode* node);
        void setLabelDeclaration(CNode* node);
        void addTempDeclaration(CNode* node);
        void addThenStatement(CNode* node);
        void addElseStatement(CNode* node);
        void removeResultDeclaration();

        auto getLabel() const
        {
            return label;
        }

        auto* getThenStatements() const
        {
            return thenStatements;
        }

        auto* getElseStatements() const
        {
            return elseStatements;
        }

        auto* getCondition() const
        {
            return condition;
        }

        void setCondition(CNode* expression)
        {
            // it is the caller's responsibilty to delete the old expression if requiered.
            condition = expression;
            condition->link(this);
        }

        void deleteCondition()
        {
            delete condition;
            condition = nullptr;
        }

    private:
        CNode* condition = nullptr;
        unsigned label = 0;
        std::vector<ValueType> types;
        CNode* resultDeclaration = nullptr;
        CNode* labelDeclaration = nullptr;
        CCompound* tempDeclarations = nullptr;
        CCompound* thenStatements = nullptr;
        CCompound* elseStatements = nullptr;
};

class CLoad : public CNode
{
    public:
        static const CNodeKind kind = kLoad;

        CLoad(std::string_view name, std::string_view memory, CNode* offset)
            : CNode(kind), name(name), memory(memory), offset(offset)
        {
            offset->link(this);
        }

        virtual bool hasSideEffects() const override
        {
            return offset->hasSideEffects();
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getName() const
        {
            return name;
        }

        std::string_view getMemory() const
        {
            return memory;
        }

        auto* getOffset() const
        {
            return offset;
        }

    private:
        std::string_view name;
        std::string memory;
        CNode* offset = nullptr;
};

class CNameUse : public CNode
{
    public:
        static const CNodeKind kind = kNameUse;

        CNameUse(std::string name)
            : CNode(kind),name(std::move(name))
        {
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        std::string_view getName() const
        {
            return name;
        }

        virtual bool hasSideEffects() const override
        {
            return false;
        }

    private:
        std::string name;
};

class CReturn : public CNode
{
    public:
        static const CNodeKind kind = kReturn;

        CReturn(CNode* value = nullptr)
            : CNode(kind), value(value)
        {
            if (value != nullptr) {
                value->link(this);
            }
        }

        auto* getValue() const
        {
            return value;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

    private:
        CNode* value = nullptr;
};

class CStore : public CNode
{
    public:
        static const CNodeKind kind = kStore;

        CStore(std::string_view name, std::string_view memory, CNode* offset, CNode* value)
            : CNode(kind), name(name), memory(memory), offset(offset), value(value)
        {
            offset->link(this);
            value->link(this);
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        auto getName() const
        {
            return name;
        }

        std::string_view getMemory() const
        {
            return memory;
        }

        auto* getOffset() const
        {
            return offset;
        }

        auto* getValue() const
        {
            return value;
        }

    private:
        std::string_view name;
        std::string memory;
        CNode* offset = nullptr;
        CNode* value = nullptr;
};

class CTernaryExpression : public CNode
{
    public:
        static const CNodeKind kind = kTernaryExpression;

        CTernaryExpression(CNode* condition, CNode* trueExpression, CNode* falseExpression)
            : CNode(kind), condition(condition), trueExpression(trueExpression),
              falseExpression(falseExpression)
        {
            condition->link(this);
            trueExpression->link(this);
            falseExpression->link(this);
        }

        auto* getCondition() const
        {
            return condition;
        }

        auto* getTrueExpression() const
        {
            return trueExpression;
        }

        auto* getFalseExpression() const
        {
            return falseExpression;
        }

        virtual bool hasSideEffects() const override
        {
            return condition->hasSideEffects() || trueExpression->hasSideEffects() ||
                falseExpression->hasSideEffects();;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

    private:
        CNode* condition = nullptr;
        CNode* trueExpression = nullptr;
        CNode* falseExpression = nullptr;
};

class CUnaryExpression : public CNode
{
    public:
        static const CNodeKind kind = kUnaryExpression;

        CUnaryExpression(std::string_view op, CNode* operand)
            : CNode(kind), op(op), operand(operand)
        {
            operand->link(this);
        }

        auto getOp() const
        {
            return op;
        }

        auto* getOperand() const
        {
            return operand;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        virtual bool hasSideEffects() const override
        {
            return operand->hasSideEffects();
        }

    private:
        std::string_view op;
        CNode* operand = nullptr;
};

class CPostfixExpression : public CNode
{
    public:
        static const CNodeKind kind = kPostfixExpression;

        CPostfixExpression(std::string_view op, CNode* operand)
            : CNode(kind), op(op), operand(operand)
        {
            operand->link(this);
        }

        auto getOp() const
        {
            return op;
        }

        auto* getOperand() const
        {
            return operand;
        }

        virtual bool equals(CNode* other) override;
        virtual void generateC(std::ostream& os, CGenerator& generator) override;

        virtual bool hasSideEffects() const override
        {
            return operand->hasSideEffects();
        }

    private:
        std::string_view op;
        CNode* operand = nullptr;
};

class CGenerator
{
    public:
        struct LabelData
        {
            CNode* declaration = 0;
            unsigned useCount = 0;
        };

        using LabelMap = std::map<unsigned, LabelData>;

        CGenerator(const Module* module, CodeEntry* codeEntry, bool enhanced = false);
        ~CGenerator();

        void generateC(std::ostream& os);

        void indent()
        {
            indentString.append("  ");
        }

        void undent()
        {
            if (indentString.size() > 1) {
                indentString.resize(indentString.size() - 2);
            }
        }

        void nl(std::ostream& os)
        {
            os << '\n' << indentString;
        }

        auto* getCodeEntry() const
        {
            return codeEntry;
        }

        auto getEnhanced() const
        {
            return enhanced;
        }

        const auto* getModule() const
        {
            return module;
        }

        void generateStatement(std::ostream& os, CNode* statement);
        void decrementUseCount(unsigned label);

    private:
        std::string localName(Instruction* instruction);
        std::string globalName(Instruction* instruction);

        void generateCFunction();
        CNode* generateCStatement();
        CCompound* saveBlockResults(uint32_t index);
        void pushBlockResults(uint32_t index);
        CNode* makeCombinedOffset(Instruction* instruction);
        std::vector<ValueType> getBlockResults(InstructionBlock* blockInstruction);
        CCompound* makeBlockResults(const std::vector<ValueType>& types);
        const Local* getLocal(uint32_t index);

        CNode* generateCBinaryExpression(std::string_view op);
        CNode* generateCBlock(Instruction* instruction);
        CNode* generateCBranch(uint32_t index);
        CNode* generateCBranch(Instruction* instruction);
        CNode* generateCBrIf(CNode* condition, uint32_t index);
        CNode* generateCBrIf(Instruction* instruction);
        CNode* generateCBrUnless(Instruction* instruction);
        CNode* generateCBrTable(Instruction* instruction);
        CNode* generateCCallPredef(std::string_view name, unsigned argumentCount);
        CNode* generateCCast(std::string_view name);
        CNode* generateCDoubleCast(std::string_view name1, std::string_view name2);
        CNode* generateCDrop(Instruction* instruction);
        CNode* generateCExtractLane(Instruction* instruction, const char* type);
        CNode* generateCReplaceLane(Instruction* instruction, const char* type);
        void generateCGlobalGet(Instruction* instruction);
        CNode* generateCGlobalSet(Instruction* instruction);
        CNode* generateCIf(Instruction* instruction);
        void generateCLoad(std::string_view name, Instruction* instruction, ValueType type);
        CNode* generateCLoadSplat(std::string_view splatName, std::string_view loadName,
                Instruction* instruction, ValueType type);
        CNode* generateCLoadExtend(std::string_view splatName, Instruction* instruction);
        void generateCLocalGet(Instruction* instruction);
        CNode* generateCLocalSet(Instruction* instruction);
        CNode* generateCLocalTee(Instruction* instruction);
        CNode* generateCLoop(Instruction* instruction);
        CNode* generateCMemorySize();
        CNode* generateCMemoryCall(std::string_view name, unsigned argumentCount);
        CNode* generateCMemoryInit(Instruction* instruction);
        CNode* generateCMemoryCopy(Instruction* instruction);
        CNode* generateCTableSize(Instruction* instruction);
        CNode* generateCTableCall(Instruction* instruction, std::string_view name, unsigned argumentCount);
        CNode* generateCTableInit(Instruction* instruction);
        CNode* generateCTableAccess(Instruction* instruction, bool set);
        CNode* generateCTableCopy(Instruction* instruction);
        CNode* generateCReturn(Instruction* instruction);
        void generateCSelect(Instruction* instruction);
        CNode* generateCShift(std::string_view op, std::string_view type);
        CNode* generateCShuffle(Instruction* instruction);
        CNode* generateCStore(std::string_view name, Instruction* instruction);
        CNode* generateCUBinaryExpression(std::string_view op, std::string_view type);
        CNode* generateCUnaryExpression(std::string_view op);
        CNode* generateCCall(Instruction* instruction);
        CNode* generateCCallIndirect(Instruction* instruction);
        CNode* generateCFunctionReference(Instruction* instruction);

        void buildCTree();
        void skipUnreachable(unsigned count = 0);

        unsigned pushLabel(std::vector<ValueType> types);
        void popLabel();

        void tempify();
        void pushExpression(CNode* expression, ValueType type = ValueType::void_, bool hasSideEffects = false);
        CNode* popExpression();
        CNode* getExpression(size_t offset);
        void replaceExpression(CNode* expression, size_t offset);

        std::string getTemp(ValueType type);
        std::vector<std::string> getTemps(const std::vector<ValueType>& types);
        void enhance();

        auto& getLabel(size_t index = 0)
        {
            assert(labelStack.size() > index);
            return labelStack[labelStack.size() - index - 1];
        }

        struct LabelInfo
        {
            LabelInfo(unsigned label)
                : label(label)
            {
            }

            unsigned label;
            bool backward = false;
            bool branchTarget = false;
            bool impliedTarget = false;
            std::vector<ValueType> types;
            std::vector<std::string> temps;
        };

        struct ExpressionInfo
        {
            ExpressionInfo(CNode* expression, ValueType type, bool sideEffects)
                : expression(expression), type(type), hasSideEffects(sideEffects || expression->hasSideEffects())
            {
            }

            CNode* expression = nullptr;
            ValueType type = ValueType::void_;
            bool hasSideEffects = false;
        };

        std::vector<std::unique_ptr<Instruction>>::iterator instructionPointer;
        std::vector<std::unique_ptr<Instruction>>::iterator instructionEnd;

        unsigned temp = 0;
        CCompound* tempNode = nullptr;
        CCompound* currentCompound = nullptr;
        unsigned label = 0;
        std::string indentString = "";
        const Module* module;
        std::vector<ExpressionInfo> expressionStack;
        CodeEntry* codeEntry;
        bool enhanced = false;
        CFunction* function;
        std::vector<LabelInfo> labelStack;
        LabelMap labelMap;
};

};

#endif
