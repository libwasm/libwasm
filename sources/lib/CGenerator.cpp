// CGenerator.cpp

#include "CGenerator.h"

#include "BackBone.h"
#include "Instruction.h"
#include "Module.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>

namespace libwasm
{

CNode::~CNode()
{
    unlink();
}

void CNode::link(CNode* p)
{
    if (parent == p && next == nullptr) {
        return;
    }

    if (parent != nullptr) {
        unlink();
    }

    if (p == nullptr) {
        return;
    }

    // check for circular link

    for (CNode* f = p; f; f = f->parent) {
        if (f == this) {
            std::cerr << "Attempt to link a node beneath itself" << std::endl;
            exit(1);
        }
    }

    parent = p;
    next = nullptr;

    previous = parent->lastChild;
    parent->lastChild = this;

    if (previous != nullptr) {
        previous->next = this;
    } else {
        parent->child = this;
    }
}

void CNode::link(CNode* p, CNode* n)
{           
    if (n == nullptr) {
        link(p);
        return;
    }
        
    if (parent == p && next == n) {
        return;
    }

    if (parent != nullptr) {
        unlink();
    }   

    // check for circular link

    for (CNode* f = p; f; f = f->parent) {
        if (f == this) {
            std::cerr << "Attempt to link a node beneath itself" << std::endl;
            exit(1);
        }
    }

    if (p == nullptr) {
        p = n->parent;
    } else if (p != n->parent) {
        std::cerr << "Attempt to link a node in 2 chains" << std::endl;
        exit(1);
    }

    parent = p;
    previous = n->previous;

    if (previous != nullptr) {
        previous->next = this;
    } else {
        p->child = this;
    }

    n->previous = this;
    next = n;
}

void CNode::linkFirst(CNode* p)
{
    link(p, p->child);
}

void CNode::unlink()
{
    if (parent == nullptr) {
        next = nullptr;
        previous = nullptr;
        return;
    }

    if (parent->child == this) {
        parent->child = next;
    }

    if (parent->lastChild == this) {
        parent->lastChild = previous;
    }

    if (previous != nullptr) {
        previous->next = next;
    }

    if (next != nullptr) {
        next->previous = previous;
    }

    parent = nullptr;
    next = nullptr;
    previous = nullptr;
}

static std::optional<int64_t> getIntegerValue(CNode* node)
{
    if (node->kind == CNode::kI32) {
        return static_cast<CI32*>(node)->value;
    } else if (node->kind == CNode::kI64) {
        return static_cast<CI64*>(node)->value;
    } else {
        return {};
    }
}


CFunction::~CFunction()
{
    for (auto* statement : statements) {
        delete statement;
    }
}

CNode* CFunction::popExpression()
{
    assert(lastChild != nullptr);

    auto* result = lastChild;

    lastChild->unlink();

    return result;
}

CNode* CFunction::pushExpression(CNode* expression)
{
    expression->link(this);
    return expression;
}

void CFunction::generateC(std::ostream& os, CGenerator* generator)
{
    generator->indent();
    for (auto& local : generator->getCodeEntry()->getLocals()) {
        os << "\n  " << local->getType().getCName() << ' ' << local->getCName() << ';';
    }

    for (auto* statement : statements) {
        generator->generateStatement(os, statement);
    }

    generator->undent();
}

void CLabel::generateC(std::ostream& os, CGenerator* generator)
{
    os << "\n" << name << ':';
}

static unsigned binaryPrecedence(std::string_view op)
{
    static struct {
        std::string_view op;
        unsigned precedence;
    } precedences[] = {
        { "*", 14 },
        { "/", 14 },
        { "%", 14 },
        { "+", 13 },
        { "-", 13 },
        { "<<", 11 },
        { ">>", 11 },
        { ">", 10 },
        { "<", 10 },
        { "<=", 10 },
        { ">=", 10 },
        { "==", 9 },
        { "!=", 9 },
        { "&", 8 },
        { "^", 7 },
        { "|", 6 },
        { "&&", 5 },
        { "||", 4 },
        { "?", 3 },
        { ":", 3 },
        { "=", 2 },
        { "*=", 2 },
        { "/=", 2 },
        { "%=", 2 },
        { "+=", 2 },
        { "-=", 2 },
        { "<<=", 2 },
        { ">>=", 2 },
        { "&=", 2 },
        { "^=", 2 },
        { "|=", 2 },
    };

    for (const auto& precedence : precedences) {
        if (op == precedence.op) {
            return precedence.precedence;
        }
    }

    assert(false);
    return 0;
}


static bool needsParenthesis(CNode* node, std::string_view op)
{
    auto* parent = node->parent;

    if (parent == nullptr) {
        return false;
    }

    auto precedence = binaryPrecedence(op);

    if (precedence == 2) {
        return true;
    }

    auto kind = parent->kind;

    if (kind == CNode::kCast) {
        return true;
    }

    if (kind == CNode::kBinauryExpression) {
        auto* binaryParent = static_cast<CBinauryExpression*>(parent);
        std::string_view parentOp = binaryParent->op;


        if (parentOp == op && (op == "+" || op == "*")) {
            return false;
        }

        auto parentPrecedence = binaryPrecedence(parentOp);

        if (parentPrecedence < 9) {
            return parentPrecedence != 2;
        }

        return parentPrecedence > precedence;
    }

    if (kind == CNode::kUnaryExpression) {
        return true;
    }

    if (kind == CNode::kTernaryExpression) {
        return true;
    }

    return false;
}

CBinauryExpression::~CBinauryExpression()
{
    delete left;
    delete right;
}

void CBinauryExpression::generateC(std::ostream& os, CGenerator* generator)
{
    bool parenthesis = needsParenthesis(this, op);

    if (op == "=") {
        if (left->kind == CNode::kNameUse && right->kind == CNode::kBinauryExpression) {
            auto* rightBinary = static_cast<CBinauryExpression*>(right);
            auto rightPrecedence = binaryPrecedence(rightBinary->op);

            if (rightPrecedence >= 13 || (rightPrecedence >= 6 && rightPrecedence <= 8)) {
                auto* rightLeft = rightBinary->left;

                if (rightLeft->kind == CNode::kNameUse) {
                    std::string_view leftName = static_cast<CNameUse*>(left)->name;
                    std::string_view rightLeftName = static_cast<CNameUse*>(rightLeft)->name;

                    if (leftName == rightLeftName) {
                        if (parenthesis) {
                            os << '(';
                        }

                        os << leftName << ' ' << rightBinary->op << "= ";
                        rightBinary->right->generateC(os, generator);

                        if (parenthesis) {
                            os << ')';
                        }

                        return;
                    }
                }
            }
        }
    }

    if (parenthesis) {
        os << '(';
    }

    left->generateC(os, generator);
    os << ' ' << op << ' ';
    right->generateC(os, generator);

    if (parenthesis) {
        os << ')';
    }
}

void CI32::generateC(std::ostream& os, CGenerator* generator)
{
    os << value;
}

void CI64::generateC(std::ostream& os, CGenerator* generator)
{
    if (value == 0x8000000000000000ULL) {
        os << "0x8000000000000000ULL";
    } else {
        os << value << "LL";
    }
}

void CNameUse::generateC(std::ostream& os, CGenerator* generator)
{
    os << name;
}

void CVariable::generateC(std::ostream& os, CGenerator* generator)
{
    os << type.getCName() << ' ' << name << " = ";

    if (initialValue == nullptr) {
        os << '0';
    } else {
        initialValue->generateC(os, generator);
    }
}

CLoad::~CLoad()
{
    delete offset;
}

void CLoad::generateC(std::ostream& os, CGenerator* generator)
{
    os << name << '(';
    offset->generateC(os, generator);
    os << ')';
}

void CBr::generateC(std::ostream& os, CGenerator* generator)
{
    os << "goto label" << label;
}

CCallIndirect::~CCallIndirect()
{
    delete tableIndex;

    for (auto* argument : arguments) {
        delete argument;
    }
}

void CCallIndirect::generateC(std::ostream& os, CGenerator* generator)
{
    os << "((type" << typeIndex << ")TABLE.functions[";
    tableIndex->generateC(os, generator);

    os << "])(";

    const char* seperator = "";

    for (auto* argument : arguments) {
        os << seperator;
        argument->generateC(os, generator);
        seperator = ", ";
    }

    os << ')';
}

CCall::~CCall()
{
    for (auto* argument : arguments) {
        delete argument;
    }
}

void CCall::generateC(std::ostream& os, CGenerator* generator)
{
    os << functionName << '(';

    const char* separator = "";
    for (auto* argument : arguments) {
        os << separator;
        argument->generateC(os, generator);
        separator = ", ";
    }

    os << ')';
}

CCast::~CCast()
{
    delete operand;
}

void CCast::generateC(std::ostream& os, CGenerator* generator)
{
    if (auto value = getIntegerValue(operand)) {
        if (*value >= 0) {
            if (type == "uint32_t" && value <= 0xffffffffffffffff) {
                os << *value << 'U';
                return;
            } else if (type == "uint64_t") {
                os << *value << "ULL";
                return;
            }
        }
    }

    os << '(' << type << ')';
    operand->generateC(os, generator);
}

void CF32::generateC(std::ostream& os, CGenerator* generator)
{
    os << toString(value);
}

void CF64::generateC(std::ostream& os, CGenerator* generator)
{
    os << toString(value);
}

CReturn::~CReturn()
{
    delete value;
}

void CReturn::generateC(std::ostream& os, CGenerator* generator)
{
    os << "return";
    if (value != nullptr) {
        os << ' ';
        value->generateC(os, generator);
    }
}

CStore::~CStore()
{
    delete value;
    delete offset;
}

void CStore::generateC(std::ostream& os, CGenerator* generator)
{
    os << name << '(';
    offset->generateC(os, generator);
    os << ", ";
    value->generateC(os, generator);
    os << ')';
}

CTernaryExpression::~CTernaryExpression()
{
    delete condition;
    delete trueExpression;
    delete falseExpression;
}

void CTernaryExpression::generateC(std::ostream& os, CGenerator* generator)
{
    condition->generateC(os, generator);
    os << " ? ";
    trueExpression->generateC(os, generator);
    os << " : ";
    falseExpression->generateC(os, generator);
}

CUnaryExpression::~CUnaryExpression()
{
    delete operand;
}

void CUnaryExpression::generateC(std::ostream& os, CGenerator* generator)
{
    os << op;
    operand->generateC(os, generator);
}

CBlock::~CBlock()
{
    for (auto* statement : statements) {
        delete statement;
    }
}

void CBlock::generateC(std::ostream& os, CGenerator* generator)
{
    for (auto* statement : statements) {
        generator->generateStatement(os, statement);
    }
}

CCompound::~CCompound()
{
    for (auto* statement : statements) {
        delete statement;
    }
}

void CCompound::generateC(std::ostream& os, CGenerator* generator)
{
    for (auto* statement : statements) {
        generator->generateStatement(os, statement);
    }
}

CLoop::~CLoop()
{
    for (auto* statement : statements) {
        delete statement;
    }
}

void CLoop::generateC(std::ostream& os, CGenerator* generator)
{
    for (auto* statement : statements) {
        generator->generateStatement(os, statement);
    }
}

CIf::~CIf()
{
    delete condition;

    for (auto* statement : thenStatements) {
        delete statement;
    }

    for (auto* statement : elseStatements) {
        delete statement;
    }
}

void CIf::generateC(std::ostream& os, CGenerator* generator)
{
    for (auto* statement : preambleStatements) {
        generator->generateStatement(os, statement);
    }

    os << "if (";
    condition->generateC(os, generator);
    os << ") {";
    generator->indent();

    for (auto* statement : thenStatements) {
        generator->generateStatement(os, statement);
    }

    generator->undent();
    generator->nl(os);
    os << '}';

    if (!elseStatements.empty()) {
        os << "else {";
        generator->indent();
        for (auto* statement : elseStatements) {
            generator->generateStatement(os, statement);
        }

        generator->undent();
        generator->nl(os);
        os << '}';
    }

    for (auto* statement : postambleStatements) {
        generator->generateStatement(os, statement);
    }

}

CSwitch::~CSwitch()
{
    delete condition;
    delete defaultCase;

    for (auto& c : cases) {
        delete c.statement;
    }
}

void CSwitch::generateC(std::ostream& os, CGenerator* generator)
{
    os << "switch (";
    condition->generateC(os, generator);
    os << ") {";

    for (auto& c : cases) {
        generator->nl(os);
        os << "  case " << c.value << ": ";
        generator->indent();
        generator->indent();
        c.statement->generateC(os, generator);
        os << ';';
        generator->undent();
        generator->undent();
    }

    if (defaultCase != nullptr) {
        generator->nl(os);
        os << "  default : ";
        generator->indent();
        generator->indent();
        defaultCase->generateC(os, generator);
        os << ';';
        generator->undent();
        generator->undent();
    }

    generator->nl(os);
    os << '}';
}

CGenerator::~CGenerator()
{
    delete function;
}

std::string CGenerator::localName(Instruction* instruction)
{
    auto* function = module->getFunction(codeEntry->getNumber());
    auto localIndex = static_cast<InstructionLocalIdx*>(instruction)->getIndex();
    auto* signature = function->getSignature();
    const auto& parameters = signature->getParams();
    std::string result;

    if (localIndex < parameters.size()) {
        result = parameters[localIndex]->getCName();
    } else {
        result = codeEntry->getLocals()[localIndex - parameters.size()]->getCName();
    }

    return result;
}

unsigned CGenerator::pushLabel(CNode* begin, ValueType type)
{
    size_t line = 999999999;

    if (instructionPointer != instructionEnd) {
        Instruction* ins = instructionPointer->get();

        if (ins != nullptr) {
            line = ins->getLineNumber();
        }
    }

//  std::cout << "Push " << label << ", size=" << labelStack.size() << ", line=" << line << std::endl;
    labelStack.emplace_back(begin, type, label);
    return label++;
}

void CGenerator::popLabel()
{
    size_t line = 999999999;

    if (instructionPointer != instructionEnd) {
        Instruction* ins = instructionPointer->get();

        if (ins != nullptr) {
            line = ins->getLineNumber();
        }
    }

//  std::cout << "Pop " << labelStack.back().label << ", size=" << labelStack.size() << ", line=" << line << std::endl;
    assert(!labelStack.empty());

    labelStack.pop_back();
}

std::string CGenerator::globalName(Instruction* instruction)
{
    auto globalIndex = static_cast<InstructionGlobalIdx*>(instruction)->getIndex();
    auto* global = module->getGlobal(globalIndex);

    return global->getCName(globalIndex);

}

CNode* CGenerator::generateCBlock(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultType = blockInstruction->getResultType();
    auto* result = new CBlock(label, resultType);
    auto blockLabel = pushLabel(result, resultType);
    std::string resultName = "result" + toString(blockLabel);
    auto lastChild = function->lastChild;
    auto labelStackSize = labelStack.size();

    if (resultType != ValueType::void_) {
        result->statements.push_back(new CVariable(resultType, resultName));
    }

    while (auto* statement = generateCStatement()) {
        result->statements.push_back(statement);
    }

    if (resultType != ValueType::void_ && function->lastChild != lastChild && lastChild != nullptr) {
        auto* resultNameNode = new CNameUse(resultName);
        auto* value = function->popExpression();

        result->statements.push_back(new CBinauryExpression(resultNameNode, value, "="));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        if (labelStack.back().branchTarget) {
            result->statements.push_back(new CLabel("label" + toString(blockLabel)));
        }

        popLabel();
    } else {
        result->statements.push_back(new CLabel("label" + toString(blockLabel)));
    }

    if (resultType != ValueType::void_) {
        function->pushExpression(new CNameUse(resultName));
    }

    return result;
}

CNode* CGenerator::generateCLoop(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultType = blockInstruction->getResultType();
    auto* result = new CLoop(label, resultType);
    auto blockLabel = pushLabel(result, resultType);
    auto labelStackSize = labelStack.size();
    std::string resultName = "result" + toString(blockLabel);
    auto lastChild = function->lastChild;

    labelStack.back().branchTarget = true;

    if (resultType != ValueType::void_) {
        result->statements.push_back(new CVariable(resultType, resultName));
    }

    result->statements.push_back(new CLabel("label" + toString(blockLabel)));

    while (auto* statement = generateCStatement()) {
        result->statements.push_back(statement);
    }

    if (resultType != ValueType::void_ && function->lastChild != lastChild && lastChild != nullptr) {
        auto* resultNameNode = new CNameUse(resultName);
        auto* value = function->popExpression();

        result->statements.push_back(new CBinauryExpression(resultNameNode, value, "="));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);

        popLabel();
    }

    if (resultType != ValueType::void_) {
        function->pushExpression(new CNameUse(resultName));
    }

    return result;
}

CNode* CGenerator::generateCIf(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultType = blockInstruction->getResultType();
    auto* condition = function->popExpression();
    auto* result = new CIf(condition, label, resultType);
    auto blockLabel = pushLabel(result, resultType);
    auto labelStackSize = labelStack.size();
    std::string resultName = "result" + toString(blockLabel);
    auto lastChild = function->lastChild;

    if (resultType != ValueType::void_) {
        result->preambleStatements.push_back(new CVariable(resultType, resultName));
    }

    while (auto* statement = generateCStatement()) {
        result->thenStatements.push_back(statement);
        if (resultType != ValueType::void_ && function->lastChild != lastChild && lastChild != nullptr) {
            auto* resultNameNode = new CNameUse(resultName);
            auto* value = function->popExpression();

            result->thenStatements.push_back(new CBinauryExpression(resultNameNode, value, "="));
        }
    }

    if (instructionPointer < instructionEnd && instructionPointer->get()->getOpcode() == Opcode::else_) {
        while (auto* statement = generateCStatement()) {
            result->elseStatements.push_back(statement);
        }

        if (resultType != ValueType::void_ && function->lastChild != lastChild && lastChild != nullptr) {
            auto* resultNameNode = new CNameUse(resultName);
            auto* value = function->popExpression();

            result->elseStatements.push_back(new CBinauryExpression(resultNameNode, value, "="));
        }
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        if (labelStack.back().branchTarget) {
            result->postambleStatements.push_back(new CLabel("label" + toString(blockLabel)));
        }

        popLabel();
    } else {
        result->postambleStatements.push_back(new CLabel("label" + toString(blockLabel)));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        popLabel();
    }

    if (resultType != ValueType::void_) {
        function->pushExpression(new CNameUse(resultName));
    }

    return result;
}

CNode* CGenerator::generateCBranchStatement(uint32_t index, bool conditional)
{
    auto& labelInfo = getLabel(index);

    labelInfo.branchTarget = true;

    if (!conditional) {
        skipUnreachable(index);
    }

    if (labelInfo.begin->kind != CNode::kLoop && labelInfo.type != ValueType::void_) {
        std::string resultName = "result" + toString(labelInfo.label);
        auto* result = function->popExpression();
        auto* assignment = new CBinauryExpression(new CNameUse(resultName), result, "=");
        auto* compound = new CCompound;

        compound->statements.push_back(assignment);
        compound->statements.push_back(new CBr(labelInfo.label));
        if (conditional) {
            function->pushExpression(new CNameUse(resultName));
        }

        return compound;
    } else {
        return new CBr(labelInfo.label);
    }
}

CNode* CGenerator::generateCBr(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(instruction);
    auto index = branchInstruction->getIndex();

    return generateCBranchStatement(index);
}

CNode* CGenerator::generateCBrIf(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(instruction);
    auto index = branchInstruction->getIndex();
    auto* condition = function->popExpression();
    auto* ifNode = new CIf(condition);

    ifNode->thenStatements.push_back(generateCBranchStatement(index, true));

    return ifNode;
}

CNode* CGenerator::generateCBrTable(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionBrTable*>(instruction);
    auto* condition = function->popExpression();

    auto defaultLabel = branchInstruction->getDefaultLabel();
    auto& defaultLabelInfo = getLabel(defaultLabel);

    defaultLabelInfo.branchTarget = true;

    if (defaultLabelInfo.type == ValueType::void_) {
        auto* result = new CSwitch(condition);

        condition->link(result);
        result->defaultCase = generateCBranchStatement(defaultLabel, true);

        unsigned count = 0;
        for (auto label : branchInstruction->getLabels()) {
            result->cases.emplace_back(count++, generateCBranchStatement(label, true));
        }

        skipUnreachable();
        return result;
    } else {
        std::string resultName = "result" + toString(label++);
        auto* result = new CCompound;
        auto* value = function->popExpression();
        auto* switchStatement = new CSwitch(condition);

        condition->link(switchStatement);

        result->statements.push_back(new CVariable(defaultLabelInfo.type, resultName, value));
        result->statements.push_back(switchStatement);

        function->pushExpression(new CNameUse(resultName));
        switchStatement->defaultCase = generateCBranchStatement(defaultLabel, true);

        unsigned count = 0;
        for (auto label : branchInstruction->getLabels()) {
            function->pushExpression(new CNameUse(resultName));
            switchStatement->cases.emplace_back(count++, generateCBranchStatement(label, true));
        }
       
        skipUnreachable();
        return result;
    }
}

CNode* CGenerator::generateCBinaryExpression(std::string_view op)
{
    auto* right = function->popExpression();
    auto* left = function->popExpression();

    return new CBinauryExpression(left, right, op);
}

CNode* CGenerator::generateCUnaryExpression(std::string_view op)
{
    auto* operand = function->popExpression();

    return new CUnaryExpression(op, operand);
}

CNode* CGenerator::generateCUBinaryExpression(std::string_view op, std::string_view type)
{
    auto* right = function->popExpression();
    auto* left = function->popExpression();

    return new CBinauryExpression(new CCast(type, left), new CCast(type, right), op);
}

CNode* CGenerator::generateCLoad(std::string_view name, Instruction* instruction)
{
    auto* memoryInstruction = static_cast<InstructionMemory*>(instruction);
    auto offset = memoryInstruction->getOffset();
    auto* dynamicOffset = function->popExpression();
    CNode* combinedOffset = nullptr;

    if (offset == 0) {
        combinedOffset = dynamicOffset;
    } else if (auto value = getIntegerValue(dynamicOffset)) {
        delete dynamicOffset;

        auto* offsetEpression = new CI64(offset);

        if (*value == 0) {
            combinedOffset = offsetEpression;
        } else {
            combinedOffset = new CBinauryExpression(offsetEpression, new CI64(*value), "+");
        }
    } else {
        combinedOffset = new CBinauryExpression(new CI64(offset), dynamicOffset, "+");
    }

    return new CLoad(name, combinedOffset);
}

CNode* CGenerator::generateCStore(std::string_view name, Instruction* instruction)
{
    auto* valueToStore = function->popExpression();
    auto* memoryInstruction = static_cast<InstructionMemory*>(instruction);
    auto offset = memoryInstruction->getOffset();
    auto* dynamicOffset = function->popExpression();
    CNode* combinedOffset = nullptr;

    if (offset == 0) {
        combinedOffset = dynamicOffset;
    } else if (auto value = getIntegerValue(dynamicOffset)) {
        delete dynamicOffset;

        auto* offsetEpression = new CI64(offset);

        if (*value == 0) {
            combinedOffset = offsetEpression;
        } else {
            combinedOffset = new CBinauryExpression(offsetEpression, new CI64(*value), "+");
        }
    } else {
        combinedOffset = new CBinauryExpression(new CI64(offset), dynamicOffset, "+");
    }

    return new CStore(name, combinedOffset, valueToStore);
}

CNode* CGenerator::generateCLocalSet(Instruction* instruction)
{
    auto* left = new CNameUse(localName(instruction));
    auto* right = function->popExpression();

    return new CBinauryExpression(left, right, "=");
}

CNode* CGenerator::generateCGlobalSet(Instruction* instruction)
{
    auto* left = new CNameUse(globalName(instruction));
    auto* right = function->popExpression();

    return new CBinauryExpression(left, right, "=");
}

CNode* CGenerator::generateCCallPredef(std::string_view name, unsigned argumentCount)
{
    auto* result = new CCall(name);

    for (unsigned i = 0; i < argumentCount; ++i) {
        auto* argument = function->popExpression();
        result->arguments.push_back(argument);
    }

    std::reverse(result->arguments.begin(), result->arguments.end());

    return result;
}

CNode* CGenerator::generateCMemoryGrow()
{
    auto* result = new CCall("growMemory");

    result->arguments.push_back(new CUnaryExpression("&", new CNameUse("MEMORY")));
    result->arguments.push_back(function->popExpression());

    return result;
}

CNode* CGenerator::generateCCast(std::string_view name)
{
    auto* operand = function->popExpression();

    return new CCast(name, operand);
}

CNode* CGenerator::generateCDoubleCast(std::string_view name1, std::string_view name2)
{
    auto* operand = function->popExpression();

    auto* cast = new CCast(name2, operand);
    return new CCast(name1, cast);
}

CNode* CGenerator::generateCDrop(Instruction* instruction)
{
    auto statement = function->popExpression();

    switch(statement->kind) {
        case CNode::kI32:
        case CNode::kI64:
        case CNode::kF32:
        case CNode::kF64:
        case CNode::kNameUse:
            return 0;

        default:
            return statement;
    }
}

CNode* CGenerator::generateCSelect(Instruction* instruction)
{
    auto* condition = function->popExpression();
    auto* falseValue = function->popExpression();
    auto* trueValue = function->popExpression();

    return new CTernaryExpression(condition, trueValue, falseValue);
}

CNode* CGenerator::generateCReturn(Instruction* instruction)
{
    auto* signature = function->signature;

    if (signature->getResults().empty()) {
        return new CReturn;
    } else {
        return new CReturn(function->popExpression());
    }

    skipUnreachable();
}

void CGenerator::generateCCall(Instruction* instruction, CNode*& expression, CNode*& statement)
{
    auto callInstruction = static_cast<InstructionFunctionIdx*>(instruction);
    auto functionIndex = callInstruction->getIndex();
    auto* calledFunction = module->getFunction(functionIndex);
    auto* signature = calledFunction->getSignature();
    auto* result = new CCall(calledFunction->getCName(functionIndex));

    for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
        auto* argument = function->popExpression();

        result->arguments.push_back(argument);
    }

    std::reverse(result->arguments.begin(), result->arguments.end());

    if (signature->getResults().empty()) {
        statement = result;
    } else {
        expression = result;
    }
}

void CGenerator::generateCCallIndirect(Instruction* instruction, CNode*& expression, CNode*& statement)
{
    auto callInstruction = static_cast<InstructionIndirect*>(instruction);
    auto typeIndex = callInstruction->getTypeIndex();
    auto* tableIndex = function->popExpression();
    auto* result = new CCallIndirect(typeIndex, tableIndex);
    auto* signature = module->getType(typeIndex)->getSignature();

    for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
        auto* argument = function->popExpression();

        result->arguments.push_back(argument);
    }

    std::reverse(result->arguments.begin(), result->arguments.end());

    if (signature->getResults().empty()) {
        statement = result;
    } else {
        expression = result;
    }
}

void CGenerator::generateCFunction()
{
    function = new CFunction;

    function->signature = module->getFunction(codeEntry->getNumber())->getSignature();

    ValueType type = ValueType::void_;

    if (!function->signature->getResults().empty()) {
        type = function->signature->getResults()[0];
        function->statements.push_back(new CVariable(type, "result0"));
    }

    pushLabel(function, type);

    while (auto* statement = generateCStatement()) {
        function->statements.push_back(statement);
    }

    if (type != ValueType::void_ && function->lastChild != nullptr) {
        auto* resultName = new CNameUse("result0");
        auto* value = function->popExpression();

        function->statements.push_back(new CBinauryExpression(resultName, value, "="));
    }

    if (labelStack.back().branchTarget) {
        function->statements.push_back(new CLabel("label0"));
    }

    if (type != ValueType::void_) {
        function->statements.push_back(new CReturn(new CNameUse("result0")));
    }

    popLabel();
    assert(labelStack.empty());
}

CNode* CGenerator::generateCStatement()
{
    while (instructionPointer < instructionEnd) {
        CNode* expression = nullptr;
        CNode* statement = nullptr;
        auto* instruction = (instructionPointer++)->get();
        auto opcode = instruction->getOpcode();

        switch(opcode) {
            case Opcode::unreachable:
                skipUnreachable();
                break;

            case Opcode::nop:
                // nothing to do
                break;

            case Opcode::block:
                statement = generateCBlock(instruction);
                break;

            case Opcode::loop:
                statement = generateCLoop(instruction);
                break;

            case Opcode::if_:
                statement = generateCIf(instruction);
                break;

            case Opcode::else_:
                return nullptr;

    //      case Opcode::try_:
    //      case Opcode::catch_:
    //      case Opcode::throw_:
    //      case Opcode::rethrow_:
    //      case Opcode::br_on_exn:

            case Opcode::end:
                return nullptr;

            case Opcode::br:
                statement = generateCBr(instruction);
                break;

            case Opcode::br_if:
                statement = generateCBrIf(instruction);
                break;

            case Opcode::br_table:
                statement = generateCBrTable(instruction);
                break;


            case Opcode::return_:
                statement = generateCReturn(instruction);
                break;

            case Opcode::call:
                generateCCall(instruction, expression, statement);
                break;

            case Opcode::call_indirect:
                generateCCallIndirect(instruction, expression, statement);
                break;

    //      case Opcode::return_call:
    //      case Opcode::return_call_indirect:

            case Opcode::drop:
                statement = generateCDrop(instruction);
                break;

            case Opcode::select:
                expression = generateCSelect(instruction);
                break;

            case Opcode::local__get:
                expression = new CNameUse(localName(instruction));
                break;

            case Opcode::local__set:
                statement = generateCLocalSet(instruction);
                break;

            case Opcode::local__tee:
                expression = generateCLocalSet(instruction);
                break;

            case Opcode::global__get:
                expression = new CNameUse(globalName(instruction));
                break;

            case Opcode::global__set:
                statement = generateCGlobalSet(instruction);
                break;

    //      case Opcode::table__get:
    //      case Opcode::table__set:

            case Opcode::i32__load:
                expression = generateCLoad("loadI32", instruction);
                break;

            case Opcode::i64__load:
                expression = generateCLoad("loadI64", instruction);
                break;

            case Opcode::f32__load:
                expression = generateCLoad("loadF32", instruction);
                break;

            case Opcode::f64__load:
                expression = generateCLoad("loadF64", instruction);
                break;

            case Opcode::i32__load8_s:
                expression = generateCLoad("loadI32I8", instruction);
                break;

            case Opcode::i32__load8_u:
                expression = generateCLoad("loadI32U8", instruction);
                break;

            case Opcode::i32__load16_s:
                expression = generateCLoad("loadI32I16", instruction);
                break;

            case Opcode::i32__load16_u:
                expression = generateCLoad("loadI32U16", instruction);
                break;

            case Opcode::i64__load8_s:
                expression = generateCLoad("loadI64I8", instruction);
                break;

            case Opcode::i64__load8_u:
                expression = generateCLoad("loadI64U8", instruction);
                break;

            case Opcode::i64__load16_s:
                expression = generateCLoad("loadI64I16", instruction);
                break;

            case Opcode::i64__load16_u:
                expression = generateCLoad("loadI64U16", instruction);
                break;

            case Opcode::i64__load32_s:
                expression = generateCLoad("loadI64I32", instruction);
                break;

            case Opcode::i64__load32_u:
                expression = generateCLoad("loadI64U32", instruction);
                break;

            case Opcode::i32__store:
                statement = generateCStore("storeI32", instruction);
                break;

            case Opcode::i64__store:
                statement = generateCStore("storeI64", instruction);
                break;

            case Opcode::f32__store:
                statement = generateCStore("storeF32", instruction);
                break;

            case Opcode::f64__store:
                statement = generateCStore("storeF64", instruction);
                break;

            case Opcode::i32__store8:
                statement = generateCStore("storeI32I8", instruction);
                break;

            case Opcode::i32__store16:
                statement = generateCStore("storeI32I16", instruction);
                break;

            case Opcode::i64__store8:
                statement = generateCStore("storeI64I8", instruction);
                break;

            case Opcode::i64__store16:
                statement = generateCStore("storeI64I16", instruction);
                break;

            case Opcode::i64__store32:
                statement = generateCStore("storeI64I32", instruction);
                break;

            case Opcode::i32__const:
                expression = new CI32(static_cast<InstructionI32*>(instruction)->getValue());
                break;

            case Opcode::i64__const:
                expression = new CI64(static_cast<InstructionI64*>(instruction)->getValue());
                break;

            case Opcode::f32__const:
                expression = new CF32(static_cast<InstructionF32*>(instruction)->getValue());
                break;

            case Opcode::f64__const:
                expression = new CF64(static_cast<InstructionF64*>(instruction)->getValue());
                break;

            case Opcode::i32__eqz:
            case Opcode::i64__eqz:
                expression = generateCUnaryExpression("!");
                break;

            case Opcode::i32__eq:
            case Opcode::i64__eq:
            case Opcode::f32__eq:
            case Opcode::f64__eq:
                expression = generateCBinaryExpression("==");
                break;

            case Opcode::i32__ne:
            case Opcode::i64__ne:
            case Opcode::f32__ne:
            case Opcode::f64__ne:
                expression = generateCBinaryExpression("!=");
                break;

            case Opcode::i32__lt_s:
            case Opcode::i64__lt_s:
            case Opcode::f32__lt:
            case Opcode::f64__lt:
                expression = generateCBinaryExpression("<");
                break;

            case Opcode::i32__lt_u:
                expression = generateCUBinaryExpression("<", "uint32_t");
                break;

            case Opcode::i64__lt_u:
                expression = generateCUBinaryExpression("<", "uint64_t");
                break;

            case Opcode::i32__gt_s:
            case Opcode::i64__gt_s:
            case Opcode::f32__gt:
            case Opcode::f64__gt:
                expression = generateCBinaryExpression(">");
                break;

            case Opcode::i32__gt_u:
                expression = generateCUBinaryExpression(">", "uint32_t");
                break;

            case Opcode::i64__gt_u:
                expression = generateCUBinaryExpression(">", "uint64_t");
                break;

            case Opcode::i32__le_s:
            case Opcode::i64__le_s:
            case Opcode::f32__le:
            case Opcode::f64__le:
                expression = generateCBinaryExpression("<=");
                break;

            case Opcode::i32__le_u:
                expression = generateCUBinaryExpression("<=", "uint32_t");
                break;

            case Opcode::i64__le_u:
                expression = generateCUBinaryExpression("<=", "uint64_t");
                break;

            case Opcode::i32__ge_s:
            case Opcode::i64__ge_s:
            case Opcode::f32__ge:
            case Opcode::f64__ge:
                expression = generateCBinaryExpression(">=");
                break;

            case Opcode::i32__ge_u:
                expression = generateCUBinaryExpression(">=", "uint32_t");
                break;

            case Opcode::i64__ge_u:
                expression = generateCUBinaryExpression(">=", "uint64_t");
                break;

            case Opcode::i32__clz:
                expression = generateCCallPredef("clz32");
                break;

            case Opcode::i32__ctz:
                expression = generateCCallPredef("ctz32");
                break;

            case Opcode::i32__popcnt:
                expression = generateCCallPredef("popcnt32");
                break;

            case Opcode::i32__add:
            case Opcode::i64__add:
            case Opcode::f32__add:
            case Opcode::f64__add:
                expression = generateCBinaryExpression("+");
                break;

            case Opcode::i32__sub:
            case Opcode::i64__sub:
            case Opcode::f32__sub:
            case Opcode::f64__sub:
                expression = generateCBinaryExpression("-");
                break;

            case Opcode::i32__mul:
            case Opcode::i64__mul:
            case Opcode::f32__mul:
            case Opcode::f64__mul:
                expression = generateCBinaryExpression("*");
                break;

            case Opcode::i32__div_s:
            case Opcode::i64__div_s:
            case Opcode::f32__div:
            case Opcode::f64__div:
                expression = generateCBinaryExpression("/");
                break;

            case Opcode::i32__div_u:
                expression = generateCUBinaryExpression("/", "uint32_t");
                break;

            case Opcode::i64__div_u:
                expression = generateCUBinaryExpression("/", "uint64_t");
                break;

            case Opcode::i32__rem_s:
            case Opcode::i64__rem_s:
                expression = generateCBinaryExpression("%");
                break;

            case Opcode::i32__rem_u:
                expression = generateCUBinaryExpression("%", "uint32_t");
                break;

            case Opcode::i64__rem_u:
                expression = generateCUBinaryExpression("%", "uint64_t");
                break;

            case Opcode::i32__and:
            case Opcode::i64__and:
                expression = generateCBinaryExpression("&");
                break;

            case Opcode::i32__or:
            case Opcode::i64__or:
                expression = generateCBinaryExpression("|");
                break;

            case Opcode::i32__xor:
            case Opcode::i64__xor:
                expression = generateCBinaryExpression("^");
                break;

            case Opcode::i32__shl:
            case Opcode::i64__shl:
                expression = generateCBinaryExpression("<<");
                break;

            case Opcode::i32__shr_s:
            case Opcode::i64__shr_s:
                expression = generateCBinaryExpression(">>");
                break;

            case Opcode::i32__shr_u:
                expression = generateCUBinaryExpression(">>", "uint32_t");
                break;

            case Opcode::i64__shr_u:
                expression = generateCUBinaryExpression(">>", "uint64_t");
                break;

            case Opcode::i32__rotl:
                expression = generateCCallPredef("rotl32", 2);
                break;

            case Opcode::i32__rotr:
                expression = generateCCallPredef("rotr32", 2);
                break;

            case Opcode::i64__clz:
                expression = generateCCallPredef("clz64");
                break;

            case Opcode::i64__ctz:
                expression = generateCCallPredef("ctz64");
                break;

            case Opcode::i64__popcnt:
                expression = generateCCallPredef("popcnt64");
                break;

            case Opcode::i64__rotl:
                expression = generateCCallPredef("rotl64", 2);
                break;

            case Opcode::i64__rotr:
                expression = generateCCallPredef("rotr64", 2);
                break;

            case Opcode::f32__neg:
            case Opcode::f64__neg:
                expression = generateCUnaryExpression("-");
                break;

            case Opcode::f32__abs:
                expression = generateCCallPredef("fabsf");
                break;

            case Opcode::f64__abs:
                expression = generateCCallPredef("fabs");
                break;

            case Opcode::f32__ceil:
                expression = generateCCallPredef("ceilf");
                break;

            case Opcode::f64__ceil:
                expression = generateCCallPredef("ceil");
                break;

            case Opcode::f32__floor:
                expression = generateCCallPredef("floorf");
                break;

            case Opcode::f64__floor:
                expression = generateCCallPredef("floor");
                break;

            case Opcode::f32__trunc:
                expression = generateCCallPredef("truncf");
                break;

            case Opcode::f64__trunc:
                expression = generateCCallPredef("trunc");
                break;

            case Opcode::f32__nearest:
                expression = generateCCallPredef("nearbyintf");
                break;

            case Opcode::f64__nearest:
                expression = generateCCallPredef("nearbyint");
                break;

            case Opcode::f32__sqrt:
                expression = generateCCallPredef("sqrtf");
                break;

            case Opcode::f64__sqrt:
                expression = generateCCallPredef("sqrt");
                break;

            case Opcode::f32__min:
                expression = generateCCallPredef("fminf");
                break;

            case Opcode::f64__min:
                expression = generateCCallPredef("fmin");
                break;

            case Opcode::f32__max:
                expression = generateCCallPredef("fmaxf");
                break;

            case Opcode::f64__max:
                expression = generateCCallPredef("fmax");
                break;

    //      case Opcode::f32__copysign:
    //      case Opcode::f64__copysign:

            case Opcode::i32__wrap_i64:
            case Opcode::i32__trunc_f32_s:
            case Opcode::i32__trunc_f64_s:
                expression = generateCCast("int32_t");
                break;

            case Opcode::i32__trunc_f32_u:
            case Opcode::i32__trunc_f64_u:
                expression = generateCDoubleCast("int32_t", "uint32_t");
                break;

            case Opcode::i64__extend_i32_s:
            case Opcode::i64__trunc_f32_s:
            case Opcode::i64__trunc_f64_s:
                expression = generateCCast("int64_t");
                break;

            case Opcode::i64__trunc_f32_u:
            case Opcode::i64__trunc_f64_u:
            case Opcode::i64__extend_i32_u:
                expression = generateCDoubleCast("int64_t", "uint32_t");
                break;

            case Opcode::f32__convert_i32_s:
            case Opcode::f32__convert_i64_s:
            case Opcode::f32__demote_f64:
                expression = generateCCast("float");
                break;

            case Opcode::f32__convert_i32_u:
                expression = generateCDoubleCast("float", "uint32_t");
                break;

            case Opcode::f32__convert_i64_u:
                expression = generateCDoubleCast("float", "uint64_t");
                break;

            case Opcode::f64__convert_i32_s:
            case Opcode::f64__convert_i64_s:
            case Opcode::f64__promote_f32:
                expression = generateCCast("double");
                break;

            case Opcode::f64__convert_i32_u:
                expression = generateCDoubleCast("float", "uint64_t");
                break;

            case Opcode::f64__convert_i64_u:
                expression = generateCDoubleCast("double", "uint64_t");
                break;

            case Opcode::i32__reinterpret_f32:
                expression = generateCCallPredef("reinterpretI32F32");
                break;

            case Opcode::i64__reinterpret_f64:
                expression = generateCCallPredef("reinterpretI64F64");
                break;

            case Opcode::f32__reinterpret_i32:
                expression = generateCCallPredef("reinterpretF32I32");
                break;

            case Opcode::f64__reinterpret_i64:
                expression = generateCCallPredef("reinterpretF64I64");
                break;

            case Opcode::memory__size:
                expression = new CNameUse("MEMORY.size");
                break;

            case Opcode::memory__grow:
                expression = generateCMemoryGrow();
                break;

    //      case Opcode::i32__extend8_s:
    //      case Opcode::i32__extend16_s:
    //      case Opcode::i64__extend8_s:
    //      case Opcode::i64__extend16_s:
    //      case Opcode::i64__extend32_s:
    //      case Opcode::ref__null:
    //      case Opcode::ref__is_null:
    //      case Opcode::ref__func:
    //      case Opcode::alloca:
    //      case Opcode::br_unless:
    //      case Opcode::call_host:
    //      case Opcode::data:
    //      case Opcode::drop_keep:
            default:
                std::cerr << "Unimplemented opcode '" << opcode << "' in generateCNode\n";
        }

        if (expression != nullptr) {
            function->pushExpression(expression);
        } else if (statement != nullptr) {
            return statement;
        }
    }

    return nullptr;
}

void CGenerator::generateStatement(std::ostream& os, CNode* statement)
{
    auto kind = statement->kind;

    if (kind != CNode::kBlock && kind != CNode::kLoop) {
        nl(os);
    }

    statement->generateC(os, this);

    if (kind != CNode::kBlock && kind != CNode::kLoop && kind != CNode::kIf && kind != CNode::kSwitch) {
        os << ';';
    }
}

void CGenerator::skipUnreachable(unsigned count)
{
    while (instructionPointer != instructionEnd) {
        auto* instruction = instructionPointer->get();
        auto opcode = instruction->getOpcode();

        if (opcode == Opcode::else_) {
            return;
        } else if (opcode == Opcode::end) {
            LabelInfo& labelInfo = labelStack.back();

            if (labelInfo.branchTarget || labelInfo.begin->kind == CNode::kIf || count-- == 0) {
                return;
            } else {
                popLabel();
            }
        }

        ++instructionPointer;
    }
}

void CGenerator::buildCTree()
{
    auto& instructions = codeEntry->getExpression()->getInstructions();

    instructionPointer = instructions.begin();
    instructionEnd = instructions.end();

    generateCFunction();
}

void CGenerator::generateC(std::ostream& os)
{
    function->generateC(os, this);
}

CGenerator::CGenerator(Module* module, CodeEntry* codeEntry)
  : module(module), codeEntry(codeEntry)
{
    buildCTree();
}

};
