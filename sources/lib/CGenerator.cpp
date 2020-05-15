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
    while (lastChild != nullptr) {
        delete lastChild;
    }

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
    if (auto* i32 = node->castTo<CI32>()) {
        return i32->getValue();
    } else if (auto* i64 = node->castTo<CI64>()) {
        return i64->getValue();
    } else {
        return {};
    }
}


void CFunction::addStatement(CNode* statement)
{
    statements->addStatement(statement);
}

void CFunction::generateC(std::ostream& os, CGenerator* generator)
{
    generator->indent();
    for (auto& local : generator->getCodeEntry()->getLocals()) {
        os << "\n  " << local->getType().getCName() << ' ' << local->getCName() << ';';
    }

    generator->generateStatement(os, statements);
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
    auto* parent = node->getParent();

    if (parent == nullptr) {
        return false;
    }

    auto parentKind = parent->getKind();

    if (parentKind == CNode::kCast) {
        return true;
    }

    if (parentKind == CNode::kBinauryExpression) {
        auto precedence = binaryPrecedence(op);

        if (precedence == 2) {
            return true;
        }

        auto* binaryParent = static_cast<CBinaryExpression*>(parent);
        std::string_view parentOp = binaryParent->getOp();


        if (parentOp == op && (op == "+" || op == "*")) {
            return false;
        }

        auto parentPrecedence = binaryPrecedence(parentOp);

        if (parentPrecedence < 9) {
            return parentPrecedence != 2;
        }

        return parentPrecedence > precedence;
    }

    if (parentKind == CNode::kUnaryExpression) {
        return true;
    }

    if (parentKind == CNode::kTernaryExpression) {
        return true;
    }

    return false;
}

void CBinaryExpression::generateC(std::ostream& os, CGenerator* generator)
{
    bool parenthesis = needsParenthesis(this, op);

    if (op == "="&& generator->getOptimized()) {
        if (left->getKind() == CNode::kNameUse && right->getKind() == CNode::kBinauryExpression) {
            auto* rightBinary = static_cast<CBinaryExpression*>(right);
            auto rightPrecedence = binaryPrecedence(rightBinary->getOp());

            if (rightPrecedence >= 13 || (rightPrecedence >= 6 && rightPrecedence <= 8)) {
                auto* rightLeft = rightBinary->left;

                if (rightLeft->getKind() == CNode::kNameUse) {
                    std::string_view leftName = static_cast<CNameUse*>(left)->getName();
                    std::string_view rightLeftName = static_cast<CNameUse*>(rightLeft)->getName();

                    if (leftName == rightLeftName) {
                        if (parenthesis) {
                            os << '(';
                        }

                        os << leftName << ' ' << rightBinary->getOp() << "= ";
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

void CCallIndirect::addArgument(CNode* argument)
{
    arguments.push_back(argument);
    argument->link(this);
}

void CCallIndirect::reverseArguments()
{
    std::reverse(arguments.begin(), arguments.end());
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

void CCall::addArgument(CNode* argument)
{
    arguments.push_back(argument);
    argument->link(this);
}

void CCall::reverseArguments()
{
    std::reverse(arguments.begin(), arguments.end());
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

void CReturn::generateC(std::ostream& os, CGenerator* generator)
{
    os << "return";
    if (value != nullptr) {
        os << ' ';
        value->generateC(os, generator);
    }
}

void CStore::generateC(std::ostream& os, CGenerator* generator)
{
    os << name << '(';
    offset->generateC(os, generator);
    os << ", ";
    value->generateC(os, generator);
    os << ')';
}

void CTernaryExpression::generateC(std::ostream& os, CGenerator* generator)
{
    condition->generateC(os, generator);
    os << " ? ";
    trueExpression->generateC(os, generator);
    os << " : ";
    falseExpression->generateC(os, generator);
}

void CUnaryExpression::generateC(std::ostream& os, CGenerator* generator)
{
    os << op;
    operand->generateC(os, generator);
}

void CCompound::addStatement(CNode* statement)
{
    statement->link(this);
}

void CCompound::generateC(std::ostream& os, CGenerator* generator)
{
    for (auto* statement = child; statement != nullptr; statement = statement->getNext()) {
        generator->generateStatement(os, statement);
    }
}

void CCompound::optimize()
{
    for (auto* statement = child; statement != nullptr; statement = statement->getNext()) {
        while (auto* compoundStatement = statement->castTo<CCompound>()) {
            if (compoundStatement->getChild() != nullptr) {
                statement = compoundStatement->getChild();

                while (auto* child = compoundStatement->getChild()) {
                    child->link(this, compoundStatement);
                }
            }

            delete compoundStatement;
        }

        if (auto* ifStatement = statement->castTo<CIf>()) {
            ifStatement->optimize();
        }
    }
}

void CIf::setResultDeclaration(CNode* node)
{
    resultDeclaration = node;
    resultDeclaration->link(this);
}

void CIf::removeResultDeclaration()
{
    delete resultDeclaration;
    resultDeclaration = nullptr;
}

void CIf::setLabelDeclaration(CNode* node)
{
    labelDeclaration = node;
    labelDeclaration->link(this);
}

void CIf::addThenStatement(CNode* statement)
{
    thenStatements->addStatement(statement);
}

void CIf::addElseStatement(CNode* statement)
{
    elseStatements->addStatement(statement);
}

void CIf::generateC(std::ostream& os, CGenerator* generator)
{
    if (resultDeclaration != nullptr) {
        generator->generateStatement(os, resultDeclaration);
    }

    os << "if (";
    condition->generateC(os, generator);
    os << ") {";
    generator->indent();

    generator->generateStatement(os, thenStatements);
    generator->undent();
    generator->nl(os);
    os << '}';

    if (!elseStatements->empty()) {
        os << "else {";
        generator->indent();
        generator->generateStatement(os, elseStatements);
        generator->undent();
        generator->nl(os);
        os << '}';
    }

    if (labelDeclaration != nullptr) {
        generator->generateStatement(os, labelDeclaration);
    }
}

void CSwitch::addCase(uint64_t value, CNode* statement)
{
    cases.emplace_back(value, statement);
    statement->link(this);
}

void CSwitch::setDefault(CNode* statement)
{
    defaultCase = statement;
    statement->link(this);
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

void CGenerator::pushExpression(CNode* expression)
{
    expressionStack.push_back(expression);
}

CNode* CGenerator::popExpression()
{
    assert(!expressionStack.empty());
    auto* result = expressionStack.back();
    expressionStack.pop_back();

    return result;
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

unsigned CGenerator::pushLabel(ValueType type)
{
    size_t line = 0;

    if (instructionPointer != instructionEnd) {
        Instruction* ins = instructionPointer->get();

        if (ins != nullptr) {
            line = ins->getLineNumber() - 1;
        }
    }

//  std::cout << "Push " << label << ", size=" << labelStack.size() << ", line=" << line << std::endl;
    labelStack.emplace_back(type, label);
    return label++;
}

void CGenerator::popLabel()
{
    size_t line = 0;

    if (instructionPointer != instructionEnd) {
        Instruction* ins = instructionPointer->get();

        if (ins != nullptr) {
            line = ins->getLineNumber() - 1;
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
    auto* result = new CCompound();
    auto blockLabel = pushLabel(resultType);
    std::string resultName = "result" + toString(blockLabel);
    auto stackSize = expressionStack.size();
    auto labelStackSize = labelStack.size();
    CNode* resultNode = nullptr;

    if (resultType != ValueType::void_) {
        result->addStatement(resultNode = new CVariable(resultType, resultName));
    }

    while (auto* statement = generateCStatement()) {
        result->addStatement(statement);
    }

    if (labelStackSize <= labelStack.size() && labelStack.back().branchTarget) {
        assert(labelStack.back().label == blockLabel);

        if (resultType != ValueType::void_ && expressionStack.size() > stackSize) {
            auto* resultNameNode = new CNameUse(resultName);

            result->addStatement(new CBinaryExpression(resultNameNode, popExpression(), "="));
        }

        result->addStatement(new CLabel("label" + toString(blockLabel)));
        popLabel();

        if (resultType != ValueType::void_) {
            pushExpression(new CNameUse(resultName));
        }
    } else if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        delete resultNode;
        popLabel();
    } else {
        delete resultNode;
    }

    return result;
}

CNode* CGenerator::generateCLoop(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultType = blockInstruction->getResultType();
    auto* result = new CCompound();
    auto blockLabel = pushLabel(resultType);
    auto labelStackSize = labelStack.size();
    std::string resultName = "result" + toString(blockLabel);
    auto stackSize = expressionStack.size();

    labelStack.back().branchTarget = true;
    labelStack.back().backward = true;

    if (resultType != ValueType::void_) {
        result->addStatement(new CVariable(resultType, resultName));
    }

    result->addStatement(new CLabel("label" + toString(blockLabel)));

    while (auto* statement = generateCStatement()) {
        result->addStatement(statement);
    }

    if (resultType != ValueType::void_ && expressionStack.size() > stackSize) {
        auto* resultNameNode = new CNameUse(resultName);
        auto* value = popExpression();

        result->addStatement(new CBinaryExpression(resultNameNode, value, "="));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);

        popLabel();
    }

    if (resultType != ValueType::void_) {
        pushExpression(new CNameUse(resultName));
    }

    return result;
}

CNode* CGenerator::generateCIf(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultType = blockInstruction->getResultType();
    auto* condition = popExpression();
    auto* result = new CIf(condition, label, resultType);
    auto blockLabel = pushLabel(resultType);
    auto labelStackSize = labelStack.size();
    std::string resultName = "result" + toString(blockLabel);
    auto stackSize = expressionStack.size();

    labelStack.back().impliedTarget = true;

    if (resultType != ValueType::void_) {
        result->setResultDeclaration(new CVariable(resultType, resultName));
    }

    while (auto* statement = generateCStatement()) {
        result->addThenStatement(statement);
        if (resultType != ValueType::void_ && expressionStack.size() > stackSize) {
            auto* resultNameNode = new CNameUse(resultName);
            auto* value = popExpression();

            result->addThenStatement(new CBinaryExpression(resultNameNode, value, "="));
        }
    }

    if (instructionPointer != instructionEnd && instructionPointer->get()->getOpcode() == Opcode::else_) {
        ++instructionPointer;
        while (auto* statement = generateCStatement()) {
            result->addElseStatement(statement);
        }

        if (resultType != ValueType::void_ && expressionStack.size() > stackSize) {
            auto* resultNameNode = new CNameUse(resultName);
            auto* value = popExpression();

            result->addElseStatement(new CBinaryExpression(resultNameNode, value, "="));
        }
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        if (labelStack.back().branchTarget) {
            result->setLabelDeclaration(new CLabel("label" + toString(blockLabel)));
        }

        popLabel();
    } else {
        result->setLabelDeclaration(new CLabel("label" + toString(blockLabel)));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        if (!labelStack.back().branchTarget) {
            result->removeResultDeclaration();
        }

        popLabel();
    }

    if (resultType != ValueType::void_) {
        pushExpression(new CNameUse(resultName));
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

    if (!labelInfo.backward && labelInfo.type != ValueType::void_) {
        std::string resultName = "result" + toString(labelInfo.label);
        auto* result = popExpression();
        auto* assignment = new CBinaryExpression(new CNameUse(resultName), result, "=");
        auto* compound = new CCompound;

        compound->addStatement(assignment);
        compound->addStatement(new CBr(labelInfo.label));
        if (conditional) {
            pushExpression(new CNameUse(resultName));
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

static CNode* notExpression(CNode* expression)
{
    if (auto* unaryExpression = expression->castTo<CUnaryExpression>()) {
        if (unaryExpression->getOp() == "!") {
            auto* operand = unaryExpression->getOperand();

            operand->unlink();

            delete expression;
            return operand;
        }
    } else if (auto* binaryExpression = expression->castTo<CBinaryExpression>()) {
        auto isRelational = true;

        auto op = binaryExpression->getOp();

        if (op == "==") {
            op = "!=";
        } else if (op == "!=") {
            op = "==";
        } else if (op == ">") {
            op = "<=";
        } else if (op == "<") {
            op = ">=";
        } else if (op == "<=") {
            op = ">";
        } else if (op == ">=") {
            op = "<";
        } else {
            isRelational = false;
        }

        if (isRelational) {
            binaryExpression->setOp(op);
            return binaryExpression;
        }
    }

    return new CUnaryExpression("!", expression);
}

CNode* CGenerator::generateCBrIf(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(instruction);
    auto index = branchInstruction->getIndex();
    auto* condition = popExpression();
    auto& labelInfo = getLabel(index);

    if (optimized && index == 0 && !labelInfo.backward && labelInfo.type == ValueType::void_) {
        auto* ifNode = new CIf(notExpression(condition));

        labelInfo.impliedTarget = true;
        
        while (auto* statement = generateCStatement()) {
            ifNode->addThenStatement(statement);
        }

        // skip back over 'end' which belongs to block, not to if.
        --instructionPointer;

        return ifNode;
    } else {
        auto* ifNode = new CIf(condition);

        ifNode->addThenStatement(generateCBranchStatement(index, true));
        return ifNode;
    }
}

CNode* CGenerator::generateCBrTable(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionBrTable*>(instruction);
    auto* condition = popExpression();
    auto defaultLabel = branchInstruction->getDefaultLabel();
    auto* result = new CSwitch(condition);

    condition->link(result);
    result->setDefault(generateCBranchStatement(defaultLabel, true));

    unsigned count = 0;
    for (auto label : branchInstruction->getLabels()) {
        result->addCase(count++, generateCBranchStatement(label, true));
    }

    skipUnreachable();
    return result;
}

CNode* CGenerator::generateCBinaryExpression(std::string_view op)
{
    auto* right = popExpression();
    auto* left = popExpression();

    return new CBinaryExpression(left, right, op);
}

CNode* CGenerator::generateCUnaryExpression(std::string_view op)
{
    auto* operand = popExpression();

    return new CUnaryExpression(op, operand);
}

CNode* CGenerator::generateCUBinaryExpression(std::string_view op, std::string_view type)
{
    auto* right = popExpression();
    auto* left = popExpression();

    return new CBinaryExpression(new CCast(type, left), new CCast(type, right), op);
}

CNode* CGenerator::generateCLoad(std::string_view name, Instruction* instruction)
{
    auto* memoryInstruction = static_cast<InstructionMemory*>(instruction);
    auto offset = memoryInstruction->getOffset();
    auto* dynamicOffset = popExpression();
    CNode* combinedOffset = nullptr;

    if (offset == 0) {
        combinedOffset = dynamicOffset;
    } else if (auto value = getIntegerValue(dynamicOffset)) {
        delete dynamicOffset;

        auto* offsetEpression = new CI64(offset);

        if (*value == 0) {
            combinedOffset = offsetEpression;
        } else {
            combinedOffset = new CBinaryExpression(offsetEpression, new CI64(*value), "+");
        }
    } else {
        combinedOffset = new CBinaryExpression(new CI64(offset), dynamicOffset, "+");
    }

    return new CLoad(name, combinedOffset);
}

CNode* CGenerator::generateCStore(std::string_view name, Instruction* instruction)
{
    auto* valueToStore = popExpression();
    auto* memoryInstruction = static_cast<InstructionMemory*>(instruction);
    auto offset = memoryInstruction->getOffset();
    auto* dynamicOffset = popExpression();
    CNode* combinedOffset = nullptr;

    if (offset == 0) {
        combinedOffset = dynamicOffset;
    } else if (auto value = getIntegerValue(dynamicOffset)) {
        delete dynamicOffset;

        auto* offsetEpression = new CI64(offset);

        if (*value == 0) {
            combinedOffset = offsetEpression;
        } else {
            combinedOffset = new CBinaryExpression(offsetEpression, new CI64(*value), "+");
        }
    } else {
        combinedOffset = new CBinaryExpression(new CI64(offset), dynamicOffset, "+");
    }

    return new CStore(name, combinedOffset, valueToStore);
}

CNode* CGenerator::generateCLocalSet(Instruction* instruction)
{
    auto* left = new CNameUse(localName(instruction));
    auto* right = popExpression();

    return new CBinaryExpression(left, right, "=");
}

CNode* CGenerator::generateCGlobalSet(Instruction* instruction)
{
    auto* left = new CNameUse(globalName(instruction));
    auto* right = popExpression();

    return new CBinaryExpression(left, right, "=");
}

CNode* CGenerator::generateCCallPredef(std::string_view name, unsigned argumentCount)
{
    auto* result = new CCall(name);

    for (unsigned i = 0; i < argumentCount; ++i) {
        auto* argument = popExpression();
        result->addArgument(argument);
    }

    result->reverseArguments();

    return result;
}

CNode* CGenerator::generateCMemoryGrow()
{
    auto* result = new CCall("growMemory");

    result->addArgument(new CUnaryExpression("&", new CNameUse("MEMORY")));
    result->addArgument(popExpression());

    return result;
}

CNode* CGenerator::generateCCast(std::string_view name)
{
    auto* operand = popExpression();

    return new CCast(name, operand);
}

CNode* CGenerator::generateCDoubleCast(std::string_view name1, std::string_view name2)
{
    auto* operand = popExpression();

    auto* cast = new CCast(name2, operand);
    return new CCast(name1, cast);
}

CNode* CGenerator::generateCDrop(Instruction* instruction)
{
    auto statement = popExpression();

    switch(statement->getKind()) {
        case CNode::kI32:
        case CNode::kI64:
        case CNode::kF32:
        case CNode::kF64:
        case CNode::kNameUse:
            delete statement;
            return 0;

        default:
            return statement;
    }
}

CNode* CGenerator::generateCSelect(Instruction* instruction)
{
    auto* condition = popExpression();
    auto* falseValue = popExpression();
    auto* trueValue = popExpression();

    return new CTernaryExpression(condition, trueValue, falseValue);
}

CNode* CGenerator::generateCReturn(Instruction* instruction)
{
    auto* signature = function->getSignature();

    if (signature->getResults().empty()) {
        return new CReturn;
    } else {
        return new CReturn(popExpression());
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
        auto* argument = popExpression();

        result->addArgument(argument);
    }

    result->reverseArguments();

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
    auto* tableIndex = popExpression();
    auto* result = new CCallIndirect(typeIndex, tableIndex);
    auto* signature = module->getType(typeIndex)->getSignature();

    for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
        auto* argument = popExpression();

        result->addArgument(argument);
    }

    result->reverseArguments();

    if (signature->getResults().empty()) {
        statement = result;
    } else {
        expression = result;
    }
}

void CGenerator::generateCFunction()
{
    function = new CFunction(module->getFunction(codeEntry->getNumber())->getSignature());

    ValueType type = ValueType::void_;
    CNode* resultNode = nullptr;

    if (!function->getSignature()->getResults().empty()) {
        type = function->getSignature()->getResults()[0];
        function->addStatement(resultNode = new CVariable(type, "result0"));
    }

    pushLabel(type);

    auto& labelInfo = labelStack.back();

    while (auto* statement = generateCStatement()) {
        function->addStatement(statement);
    }

    CNode* returnExpression = nullptr;

    if (type != ValueType::void_ && !expressionStack.empty()) {
        returnExpression = popExpression();
    }

    if (!labelStack.empty() && labelStack.back().branchTarget) {
        if (returnExpression != nullptr) {
            function->addStatement(new CBinaryExpression(new CNameUse("result0"), returnExpression, "="));
        }

        function->addStatement(new CLabel("label0"));

        if (returnExpression != nullptr) {
            function->addStatement(new CReturn(new CNameUse("result0")));
        } else {
            delete resultNode;
        }
    } else if (returnExpression != nullptr) {
        delete resultNode;
        function->addStatement(new CReturn(returnExpression));
    } else {
        delete resultNode;
    }

    popLabel();
    assert(labelStack.empty());
}

CNode* CGenerator::generateCStatement()
{
    while (instructionPointer != instructionEnd) {
        CNode* expression = nullptr;
        CNode* statement = nullptr;
        auto* instruction = instructionPointer->get();
        auto opcode = instruction->getOpcode();

        if (opcode == Opcode::else_) {
            return nullptr;
        }

        ++instructionPointer;

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
            pushExpression(expression);
        } else if (statement != nullptr) {
            return statement;
        }
    }

    return nullptr;
}

void CGenerator::generateStatement(std::ostream& os, CNode* statement)
{
    auto kind = statement->getKind();

    if (kind != CNode::kCompound) {
        nl(os);
    }

    statement->generateC(os, this);

    if (kind != CNode::kIf && kind != CNode::kSwitch && kind != CNode::kCompound) {
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

            if (labelInfo.branchTarget || labelInfo.impliedTarget || count-- == 0) {
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
    if (optimized) {
        function->optimize();
    }

    function->generateC(os, this);
}

CGenerator::CGenerator(Module* module, CodeEntry* codeEntry, bool optimized)
  : module(module), codeEntry(codeEntry), optimized(optimized)
{
    buildCTree();
}

};
