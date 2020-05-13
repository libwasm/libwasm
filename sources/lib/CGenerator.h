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
            kNameUse,
            kReturn,
            kStore,
            kSwitch,
            kTernaryExpression,
            kUnaryExpression,
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

        virtual void generateC(std::ostream& os, CGenerator* generator)
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

    protected:
        CNodeKind nodeKind;
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

        ~CCompound();

        bool empty() const
        {
            return child == nullptr;
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        void addStatement(CNode* statement);
        void optimize();
};

class CBr : public CNode
{
        static const CNodeKind kind = kBr;

    public:
        CBr(unsigned label)
            : CNode(kind), label(label)
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
        static const CNodeKind kind = kSwitch;

        CSwitch(CNode* condition)
            : CNode(kind), condition(condition)
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
        static const CNodeKind kind = kBinauryExpression;

        CBinauryExpression(CNode* left, CNode* right, std::string_view op)
            : CNode(kind), left(left), right(right), op(op)
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
        static const CNodeKind kind = kCallIndirect;

        CCallIndirect(uint32_t typeIndex, CNode* tableIndex)
            : CNode(kind), typeIndex(typeIndex), tableIndex(tableIndex)
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
        static const CNodeKind kind = kCall;

        CCall(std::string_view name)
            : CNode(kind), functionName(name)
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
        static const CNodeKind kind = kCast;

        CCast(std::string_view type, CNode* operand)
            : CNode(kind), type(type), operand(operand)
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
        static const CNodeKind kind = kLabel;

        CLabel(std::string_view name)
            : CNode(kind), name(name)
        {
        }

        virtual void generateC(std::ostream& os, CGenerator* generator);

        std::string name;
};

class CVariable : public CNode
{
    public:
        static const CNodeKind kind = kVariable;

        CVariable(ValueType type, std::string_view name, CNode* initialValue = nullptr)
            : CNode(kind), type(type), name(name), initialValue(initialValue)
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
        static const CNodeKind kind = kFunction;

        CFunction(Signature* signature)
            : CNode(kind), signature(signature)
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
        static const CNodeKind kind = kI32;

        CI32(uint32_t value)
           : CNode(kind), value(value)
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
        static const CNodeKind kind = kI64;

        CI64(uint64_t value)
           : CNode(kind), value(value)
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
        static const CNodeKind kind = kF32;

        CF32(float value)
           : CNode(kind), value(value)
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
        static const CNodeKind kind = kF64;

        CF64(double value)
           : CNode(kind), value(value)
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
        static const CNodeKind kind = kIf;

        CIf(CNode* condition, unsigned label = 0, ValueType type = ValueType::void_)
            : CNode(kind), condition(condition), label(label), type(type)
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
        static const CNodeKind kind = kLoad;

        CLoad(std::string_view name, CNode* offset)
            : CNode(kind), name(name), offset(offset)
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
        static const CNodeKind kind = kNameUse;

        CNameUse(std::string name)
            : CNode(kind),name(std::move(name))
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
        static const CNodeKind kind = kReturn;

        CReturn(CNode* value = nullptr)
            : CNode(kind), value(value)
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
        static const CNodeKind kind = kStore;

        CStore(std::string_view name, CNode* offset, CNode* value)
            : CNode(kind), name(name), offset(offset), value(value)
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
        static const CNodeKind kind = kTernaryExpression;

        CTernaryExpression(CNode* condition, CNode* trueExpression, CNode* falseExpression)
            : CNode(kind), condition(condition), trueExpression(trueExpression),
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
        static const CNodeKind kind = kUnaryExpression;

        CUnaryExpression(std::string_view op, CNode* operand)
            : CNode(kind), op(op), operand(operand)
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

        auto getOptimized() const
        {
            return optimized;
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

        unsigned pushLabel(ValueType type);
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
            LabelInfo(ValueType type, unsigned label)
                : type(type), label(label)
            {
            }

            ValueType type;
            unsigned label;
            bool backward = false;
            bool branchTarget = false;
            bool impliedTarget = false;
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
