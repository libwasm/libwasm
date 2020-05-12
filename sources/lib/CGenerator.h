// CGenerator.h

#ifndef CGENERATOR_H
#define CGENERATOR_H

#include "Encodings.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace libwasm
{

class CGenerator;

class CodeEntry;
class Instruction;
class InstructionContext;
class Local;
class Module;
class Signature;

class CNode
{
    public:
        enum CNodeKind
        {
            kBinauryExpression,
            kBlock,
            kBr,
            kCallIndirect,
            kCall,
            kCast,
            kCompound,
            kFunction,
            kI32,
            kI64,
            kF32,
            kF64,
            kIf,
            kLabel,
            kLoad,
            kLoop,
            kNameUse,
            kReturn,
            kStore,
            kSwitch,
            kTernaryExpression,
            kUnaryExpression,
            kVariable,
        };

        CNode(CNodeKind kind)
          : kind(kind)
        {
        }

        virtual ~CNode();

        void link(CNode* p);
        void link(CNode* parent, CNode* n);
        void linkFirst(CNode* p);

        void unlink();

        virtual void generateC(std::ostream& os, CGenerator* generator)
        {
            std::cerr << "Not implemented kind " << kind << std::endl;
        }

        auto getKind() const
        {
            return kind;
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

    protected:
        CNodeKind kind;
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
        CCompound()
            : CNode(kCompound)
        {
        }

        ~CCompound();

        bool empty() const
        {
            return child == nullptr;
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void addStatement(CNode* statement);
        void optimize();
};

class CBlock : public CNode
{
    public:
        CBlock(unsigned label, ValueType type)
            : CNode(kBlock), label(label), type(type)
        {
            statements = new CCompound;
            statements->link(this);
        }

        ~CBlock();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void addStatement(CNode* statement);

        auto getLabel() const
        {
            return label;
        }

        void optimize()
        {
            statements->optimize();
        }

    private:
        unsigned label = 0;
        ValueType type = ValueType::void_;
        CCompound* statements = nullptr;
};

class CLoop : public CNode
{
    public:
        CLoop(unsigned label, ValueType type)
            : CNode(kLoop), label(label), type(type)
        {
            statements = new CCompound;
            statements->link(this);
        }

        ~CLoop();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void addStatement(CNode* statement);

        auto getLabel() const
        {
            return label;
        }

        void optimize()
        {
            statements->optimize();
        }

    private:
        unsigned label = 0;
        ValueType type = ValueType::void_;
        CCompound* statements = nullptr;
};

class CBr : public CNode
{
    public:
        CBr(unsigned label)
            : CNode(kBr), label(label)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        auto getLabel() const
        {
            return label;
        }

    private:
        unsigned label = 0;
};

class CSwitch : public CNode
{
    public:
        CSwitch(CNode* condition)
            : CNode(kSwitch), condition(condition)
        {
            condition->link(this);
        }

        ~CSwitch();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        struct Case
        {
            Case(uint64_t value, CNode* statement)
              : value(value), statement(statement)
            {
            }

            uint64_t value = 0;
            CNode* statement = nullptr;
        };

        void addCase(uint64_t value, CNode* statement);
        void setDefault(CNode* statement);

    private:
        CNode* condition = nullptr;
        std::vector<Case> cases;
        CNode* defaultCase = nullptr;
};

class CBinauryExpression : public CNode
{
    public:
        CBinauryExpression(CNode* left, CNode* right, std::string_view op)
            : CNode(kBinauryExpression), left(left), right(right), op(op)
        {
            left->link(this);
            right->link(this);
        }

        ~CBinauryExpression();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        auto getOp() const
        {
            return op;
        }

    private:
        CNode* left = nullptr;
        CNode* right = nullptr;
        std::string_view op;
};

class CCallIndirect : public CNode
{
    public:
        CCallIndirect(uint32_t typeIndex, CNode* tableIndex)
            : CNode(kCallIndirect), typeIndex(typeIndex), tableIndex(tableIndex)
        {
        }

        ~CCallIndirect();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void addArgument(CNode* argument);
        void reverseArguments();

    private:
        uint32_t typeIndex = 0;
        CNode* tableIndex = nullptr;
        std::vector<CNode*> arguments;
};

class CCall : public CNode
{
    public:
        CCall(std::string_view name)
            : CNode(kCall), functionName(name)
        {
        }

        ~CCall();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void addArgument(CNode* argument);
        void reverseArguments();

    private:
        std::string functionName;
        std::vector<CNode*> arguments;
};

class CCast : public CNode
{
    public:
        CCast(std::string_view type, CNode* operand)
            : CNode(kCast), type(type), operand(operand)
        {
            operand->link(this);
        }

        ~CCast();

        virtual void generateC(std::ostream& os, CGenerator* generator);

    private:
        std::string_view type;
        CNode* operand = nullptr;
};

class CLabel : public CNode
{
    public:
        CLabel(std::string_view name)
            : CNode(kLabel), name(name)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        std::string name;
};

class CVariable : public CNode
{
    public:
        CVariable(ValueType type, std::string_view name, CNode* initialValue = nullptr)
            : CNode(kVariable), type(type), name(name), initialValue(initialValue)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

    private:
        ValueType type;
        std::string name;
        CNode* initialValue = nullptr;
};

class CFunction : public CNode
{
    public:
        CFunction(Signature* signature)
            : CNode(kFunction), signature(signature)
        {
            statements = new CCompound;
            statements->link(this);
        }

        ~CFunction();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void addStatement(CNode* statement);

        auto* getSignature() const
        {
            return signature;
        }

        void optimize()
        {
            statements->optimize();
        }

    private:
        std::string name;
        Signature* signature = nullptr;
        CCompound* statements = nullptr;
};

class CI32 : public CNode
{
    public:
        CI32(uint32_t value)
           : CNode(kI32), value(value)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        auto getValue() const
        {
            return value;
        }

    private:
        int32_t value = 0;
};

class CI64 : public CNode
{
    public:
        CI64(uint64_t value)
           : CNode(kI64), value(value)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        auto getValue() const
        {
            return value;
        }

    private:
        int64_t value = 0;
};

class CF32 : public CNode
{
    public:
        CF32(float value)
           : CNode(kF32), value(value)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        auto getValue() const
        {
            return value;
        }

    private:
        float value = 0;
};

class CF64 : public CNode
{
    public:
        CF64(double value)
           : CNode(kF64), value(value)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        auto getValue() const
        {
            return value;
        }

    private:
        double value = 0;
};

class CIf : public CNode
{
    public:
        CIf(CNode* condition, unsigned label = 0, ValueType type = ValueType::void_)
            : CNode(kIf), condition(condition), label(label), type(type)
        {
            thenStatements = new CCompound;
            thenStatements->link(this);
            elseStatements = new CCompound;
            elseStatements->link(this);
        }

        ~CIf();

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void setResultDeclaration(CNode* node);
        void setLabelDeclaration(CNode* node);
        void addThenStatement(CNode* node);
        void addElseStatement(CNode* node);
        void removeResultDeclaration();

        auto getLabel() const
        {
            return label;
        }

        void optimize()
        {
            thenStatements->optimize();
            elseStatements->optimize();
        }

    private:
        CNode* condition = nullptr;
        unsigned label = 0;
        ValueType type = ValueType::void_;
        CNode* resultDeclaration = nullptr;
        CNode* labelDeclaration = nullptr;
        CCompound* thenStatements = nullptr;
        CCompound* elseStatements = nullptr;
};

class CLoad : public CNode
{
    public:
        CLoad(std::string_view name, CNode* offset)
            : CNode(kLoad), name(name), offset(offset)
        {
        }

        ~CLoad();

        virtual void generateC(std::ostream& os, CGenerator* generator);

    private:
        std::string_view name;
        CNode* offset = nullptr;
};

class CNameUse : public CNode
{
    public:
        CNameUse(std::string name)
            : CNode(kNameUse),name(std::move(name))
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        std::string_view getName() const
        {
            return name;
        }

    private:
        std::string name;
};

class CReturn : public CNode
{
    public:
        CReturn(CNode* value = nullptr)
            : CNode(kReturn), value(value)
        {
        }

        ~CReturn();

        virtual void generateC(std::ostream& os, CGenerator* generator);

    private:
        CNode* value = nullptr;
};

class CStore : public CNode
{
    public:
        CStore(std::string_view name, CNode* offset, CNode* value)
            : CNode(kStore), name(name), offset(offset), value(value)
        {
        }

        ~CStore();

        virtual void generateC(std::ostream& os, CGenerator* generator);

    private:
        std::string_view name;
        CNode* offset = nullptr;
        CNode* value = nullptr;
};

class CTernaryExpression : public CNode
{
    public:
        CTernaryExpression(CNode* condition, CNode* trueExpression, CNode* falseExpression)
            : CNode(kTernaryExpression), condition(condition), trueExpression(trueExpression),
              falseExpression(falseExpression)
        {
            condition->link(this);
            trueExpression->link(this);
            falseExpression->link(this);
        }

        ~CTernaryExpression();

        virtual void generateC(std::ostream& os, CGenerator* generator);

    private:
        CNode* condition = nullptr;
        CNode* trueExpression = nullptr;
        CNode* falseExpression = nullptr;
};

class CUnaryExpression : public CNode
{
    public:
        CUnaryExpression(std::string_view op, CNode* operand)
            : CNode(kUnaryExpression), op(op), operand(operand)
        {
            operand->link(this);
        }

        ~CUnaryExpression();

        virtual void generateC(std::ostream& os, CGenerator* generator);

    private:
        std::string_view op;
        CNode* operand = nullptr;
};

class CGenerator
{
    public:
        CGenerator(Module* module, CodeEntry* codeEntry, bool optimized = false);
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

        void generateStatement(std::ostream& os, CNode* statement);

    private:
        std::string localName(Instruction* instruction);
        std::string globalName(Instruction* instruction);

        void generateCFunction();
        CNode* generateCStatement();
        CNode* generateCBranchStatement(uint32_t index, bool conditional = false);

        CNode* generateCBinaryExpression(std::string_view op);
        CNode* generateCBlock(Instruction* instruction);
        CNode* generateCBr(Instruction* instruction);
        CNode* generateCBrIf(Instruction* instruction);
        CNode* generateCBrTable(Instruction* instruction);
        CNode* generateCCallPredef(std::string_view name, unsigned argumentCount = 1);
        CNode* generateCCast(std::string_view name);
        CNode* generateCDoubleCast(std::string_view name1, std::string_view name2);
        CNode* generateCDrop(Instruction* instruction);
        CNode* generateCGlobalSet(Instruction* instruction);
        CNode* generateCIf(Instruction* instruction);
        CNode* generateCLoad(std::string_view name, Instruction* instruction);
        CNode* generateCLocalSet(Instruction* instruction);
        CNode* generateCLoop(Instruction* instruction);
        CNode* generateCMemoryGrow();
        CNode* generateCReturn(Instruction* instruction);
        CNode* generateCSelect(Instruction* instruction);
        CNode* generateCStore(std::string_view name, Instruction* instruction);
        CNode* generateCUBinaryExpression(std::string_view op, std::string_view type);
        CNode* generateCUnaryExpression(std::string_view op);
        void generateCCall(Instruction* instruction, CNode*& expression, CNode*& statement);
        void generateCCallIndirect(Instruction* instruction, CNode*& expression, CNode*& statement);

        void buildCTree();
        void skipUnreachable(unsigned count = 0);

        unsigned pushLabel(CNode* begin, ValueType type);
        void popLabel();

        void pushExpression(CNode* expression);
        CNode* popExpression();

        auto& getLabel(size_t index = 0)
        {
            assert(labelStack.size() > index);
            return labelStack[labelStack.size() - index - 1];
        }

        struct LabelInfo
        {
            LabelInfo(CNode* begin, ValueType type, unsigned label)
                : begin(begin), type(type), label(label)
            {
            }

            CNode* begin;
            ValueType type;
            unsigned label;
            bool branchTarget = false;
        };

        std::vector<std::unique_ptr<Instruction>>::iterator instructionPointer;
        std::vector<std::unique_ptr<Instruction>>::iterator instructionEnd;

        unsigned label = 0;
        std::string indentString = "";
        Module* module;
        std::vector<CNode*> expressionStack;
        bool optimized = false;
        CodeEntry* codeEntry;
        CFunction* function;
        std::vector<LabelInfo> labelStack;
};

};

#endif
