// CGenerator.cpp

#include "CGenerator.h"

#include "BackBone.h"
#include "Instruction.h"
#include "Module.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <optional>

using namespace std::string_literals;

namespace libwasm
{

static void traverse(CNode* node, std::function<void(CNode*)> exec)
{
    for (auto* next = node->getChild(); next != nullptr; next = next->traverseToNext(node)) {
        exec(next);
    }
}

static void traverseStatements(CCompound* node, std::function<void(CNode*)> exec)
{
    for (auto* next = node->getChild(); next != nullptr; next = next->getNext()) {
        exec(next);

        if (auto* ifStatement = next->castTo<CIf>(); ifStatement != nullptr) {
            traverseStatements(ifStatement->getThenStatements(), exec);
            traverseStatements(ifStatement->getElseStatements(), exec);
        } else if (auto* switchStatement = next->castTo<CSwitch>(); switchStatement != nullptr) {
            for (auto& cs : switchStatement->getCases()) {
                auto* statement = cs.statement;

                if (auto* compound = statement->castTo<CCompound>(); compound != nullptr) {
                    traverseStatements(compound, exec);
                }

                exec(statement);
            }
        } else if (auto* compound = next->castTo<CCompound>(); compound != nullptr) {
            traverseStatements(compound, exec);
        }
    }
}

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

CNode* CNode::findNext(CNode::CNodeKind kind)
{
    CNode* node;

    for (node = next; node != 0 && node->nodeKind != kind; node = node->next) {
        // nop
    }

    return node;
}

CNode* CNode::traverseToNext(CNode* root)
{
    const auto* current = this;
    auto* result = child;

    if (result == nullptr) {
        if (root == this) {
            return nullptr;
        }

        result = next;
    }

    while (result == nullptr) {
        if (current = current->parent; current == nullptr || current == root) {
            return nullptr;
        }

        result = current->next;
    }

    return result;
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

void CFunction::generateC(std::ostream& os, CGenerator& generator)
{
    generator.indent();
    for (auto& local : generator.getCodeEntry()->getLocals()) {
        os << "\n  " << local->getType().getCName() << ' ' << local->getCName() << ';';
    }

    generator.generateStatement(os, statements);
    generator.undent();
}

void CFunction::optimize(CGenerator& generator)
{
    statements->optimize(generator);
    statements->flatten();
}

void CLabel::generateC(std::ostream& os, CGenerator& generator)
{
    os << "\nlabel" << toString(label) << ':';
    if (next == nullptr) {
        os << ';';
    }
}

static unsigned binaryPrecedence(std::string_view op)
{
    static struct {
        std::string_view op;
        unsigned precedence;
    } precedences[] = {
        { ".", 15 },
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

        if (precedence == 11) {
            return true;
        }

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

void CBinaryExpression::generateC(std::ostream& os, CGenerator& generator)
{
    bool parenthesis = needsParenthesis(this, op);

    if (op == "="&& generator.getOptimized()) {
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

    if (op == ".") {
        os << op;
    } else {
        os << ' ' << op << ' ';
    }

    right->generateC(os, generator);

    if (parenthesis) {
        os << ')';
    }
}

void CI32::generateC(std::ostream& os, CGenerator& generator)
{
    os << value;
}

void CI64::generateC(std::ostream& os, CGenerator& generator)
{
    if (value == 0x8000000000000000ULL) {
        os << "0x8000000000000000ULL";
    } else {
        os << value << "LL";
    }
}

void CNameUse::generateC(std::ostream& os, CGenerator& generator)
{
    os << name;
}

void CVariable::generateC(std::ostream& os, CGenerator& generator)
{
    os << type.getCName() << ' ' << name << " = ";

    if (initialValue == nullptr) {
        os << type.getCNullValue();
    } else {
        initialValue->generateC(os, generator);
    }
}

void CLoad::generateC(std::ostream& os, CGenerator& generator)
{
    os << name << "(&" << memory << ", ";
    offset->generateC(os, generator);
    os << ')';
}

void CBr::generateC(std::ostream& os, CGenerator& generator)
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

void CCallIndirect::generateC(std::ostream& os, CGenerator& generator)
{
    auto* table = generator.getModule()->getTable(0);

    os << "((type" << typeIndex << ')' << table->getCName() << ".functions[";
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

void CCall::generateC(std::ostream& os, CGenerator& generator)
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

void CCast::generateC(std::ostream& os, CGenerator& generator)
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

void CF32::generateC(std::ostream& os, CGenerator& generator)
{
    os << toString(value);
}

void CF64::generateC(std::ostream& os, CGenerator& generator)
{
    os << toString(value);
}

void CV128::generateC(std::ostream& os, CGenerator& generator)
{
    union
    {
        int64_t a64[2];
        v128_t v128;
    };

    v128 = value;

    os << "v128Makei64x2(0x" << std::hex << std::setw(16) << std::setfill('0') << a64[0] <<
                    "LL, 0x" << std::setw(16) << std::setfill('0') << a64[0] << std::dec << "LL)";
}

void CReturn::generateC(std::ostream& os, CGenerator& generator)
{
    os << "return";
    if (value != nullptr) {
        os << ' ';
        value->generateC(os, generator);
    }
}

void CStore::generateC(std::ostream& os, CGenerator& generator)
{
    os << name << "(&" << memory << ", ";
    offset->generateC(os, generator);
    os << ", ";
    value->generateC(os, generator);
    os << ')';
}

void CTernaryExpression::generateC(std::ostream& os, CGenerator& generator)
{
    condition->generateC(os, generator);
    os << " ? ";
    trueExpression->generateC(os, generator);
    os << " : ";
    falseExpression->generateC(os, generator);
}

void CUnaryExpression::generateC(std::ostream& os, CGenerator& generator)
{
    os << op;
    operand->generateC(os, generator);
}

void CCompound::addStatement(CNode* statement)
{
    statement->link(this);
}

void CCompound::generateC(std::ostream& os, CGenerator& generator)
{
    for (auto* statement = child; statement != nullptr; statement = statement->getNext()) {
        generator.generateStatement(os, statement);
    }
}

void CCompound::flatten()
{
    for (auto* statement = child; statement != nullptr; ) {
        if (auto* compound = statement->castTo<CCompound>(); compound != nullptr) {
            statement = compound->getChild();

            while (compound->getChild() != nullptr) {
                compound->getChild()->link(this, compound);
            }

            delete compound;
        } else if (auto* ifStatement = statement->castTo<CIf>(); ifStatement != nullptr) {
            statement = statement->getNext();

            ifStatement->getThenStatements()->flatten();
            ifStatement->getElseStatements()->flatten();
        } else {
            statement = statement->getNext();
        }
    }
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

static CLabel* findLabel(CNode* node, unsigned label)
{
    auto *statement = node->findNext(CNode::kLabel);

    if (statement == nullptr) {
        return nullptr;
    }

    if (auto* labelStatement = statement->castTo<CLabel>();
            labelStatement == nullptr || labelStatement->getLabel() != label) {
        return nullptr;
    } else {
        return labelStatement;
    }
}

void CCompound::optimizeIfs(CGenerator& generator)
{
    traverseStatements(this, [&generator](CNode* node) {
            auto* ifStatement = node->castTo<CIf>();

            if (ifStatement == nullptr || ifStatement->getThenStatements()->empty() ||
                    !ifStatement->getElseStatements()->empty()) {
                return;
            }

            auto* branchStatement = ifStatement->getThenStatements()->getLastChild()->castTo<CBr>();

            if (branchStatement == nullptr || branchStatement->getLastChild() != nullptr) {
                return;
            }

            auto label = branchStatement->getLabel();
            auto *labelStatement = findLabel(node, label);

            if (labelStatement == nullptr) {
                return;
            }

            auto* condition = ifStatement->getCondition();

            condition->unlink();
            ifStatement->setCondition(notExpression(condition));

            delete branchStatement;

            for (auto* next = node->getNext(); ; next = node->getNext()) {
                ifStatement->addThenStatement(next);

                if (next == labelStatement) {
                    break;
                }
            }

            generator.decrementUseCount(label);

            if (branchStatement = ifStatement->getThenStatements()->getLastChild()->castTo<CBr>();
                    branchStatement != nullptr) {
                label = branchStatement->getLabel();
                labelStatement = findLabel(node->getParent(), label);

                if (labelStatement == nullptr) {
                    return;
                }

                for (auto* next = node->getParent()->getNext(); ; next = node->getParent()->getNext()) {
                    ifStatement->addElseStatement(next);

                    if (next == labelStatement) {
                        break;
                    }
                }

                delete branchStatement;
                generator.decrementUseCount(label);
            }
        });
}

void CCompound::optimize(CGenerator& generator)
{
    optimizeIfs(generator);
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

void CIf::generateC(std::ostream& os, CGenerator& generator)
{
    if (resultDeclaration != nullptr) {
        generator.generateStatement(os, resultDeclaration);
    }

    os << "if (";
    condition->generateC(os, generator);
    os << ") {";
    generator.indent();

    generator.generateStatement(os, thenStatements);
    generator.undent();
    generator.nl(os);
    os << '}';

    if (!elseStatements->empty()) {
        os << " else {";
        generator.indent();
        generator.generateStatement(os, elseStatements);
        generator.undent();
        generator.nl(os);
        os << '}';
    }

    if (labelDeclaration != nullptr) {
        generator.generateStatement(os, labelDeclaration);
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

void CSwitch::generateC(std::ostream& os, CGenerator& generator)
{
    os << "switch (";
    condition->generateC(os, generator);
    os << ") {";

    for (auto& c : cases) {
        generator.nl(os);
        os << "  case " << c.value << ": ";
        generator.indent();
        generator.indent();
        c.statement->generateC(os, generator);
        os << ';';
        generator.undent();
        generator.undent();
    }

    if (defaultCase != nullptr) {
        generator.nl(os);
        os << "  default : ";
        generator.indent();
        generator.indent();
        defaultCase->generateC(os, generator);
        os << ';';
        generator.undent();
        generator.undent();
    }

    generator.nl(os);
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

std::vector<std::string> CGenerator::getTemps(const std::vector<ValueType>& types)
{
    std::vector<std::string> result;
    result.reserve(types.size());

    for (auto& type : types) {
        auto tempName = "temp_" + toString(temp++);

        result.push_back(tempName);
        tempNode->addStatement(new CVariable(type, tempName));
    }

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

unsigned CGenerator::pushLabel(std::vector<ValueType> types)
{
    size_t line = 0;

    if (instructionPointer != instructionEnd) {
        Instruction* ins = instructionPointer->get();

        if (ins != nullptr) {
            line = ins->getLineNumber() - 1;
        }
    }

    labelStack.emplace_back(label);
    labelStack.back().types = std::move(types);
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

    return global->getCName();

}

std::vector<ValueType> CGenerator::getBlockResults(InstructionBlock* blockInstruction)
{
    std::vector<ValueType> types;

    if (auto* signature = blockInstruction->getSignature(); signature != nullptr) {
        types = signature->getResults();
    } else if (blockInstruction->getResultType() != ValueType::void_) {
        types.push_back(blockInstruction->getResultType());
    }

    return types;
}

CNode* CGenerator::makeBlockResults(const std::vector<ValueType>& types)
{
    auto label = labelStack.back().label;
    auto count = types.size();

    if (count > 0) {
        auto resultNode = new CCompound;

        for (size_t i = 0; i < count; ++i) {
            resultNode->addStatement(new CVariable(types[i], makeResultName(label, i)));
        }

        return resultNode;
    }

    return nullptr;
}

CNode* CGenerator::generateCBlock(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultTypes = getBlockResults(blockInstruction);
    auto* result = new CCompound();
    auto blockLabel = pushLabel(resultTypes);
    auto label = labelStack.back().label;
    auto stackSize = expressionStack.size();
    auto labelStackSize = labelStack.size();
    auto types = getBlockResults(blockInstruction);
    auto count = types.size();
    auto* resultNode = makeBlockResults(types);

    if (resultNode != nullptr) {
        result->addStatement(resultNode);
    }

    while (auto* statement = generateCStatement()) {
        result->addStatement(statement);
        if (labelStack.size() < labelStackSize) {
            break;
        }
    }

    if (labelStackSize <= labelStack.size() && labelStack.back().branchTarget) {
        assert(labelStack.back().label == blockLabel);

        for (auto i = count; expressionStack.size() > stackSize && i-- > 0; ) {
            auto* resultNameNode = new CNameUse(makeResultName(label, i));

            result->addStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
        }

        result->addStatement(new CLabel(blockLabel));
        popLabel();

        for (size_t i = 0; i < count; ++i) {
            pushExpression(new CNameUse(makeResultName(label, i)));
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
    auto resultTypes = getBlockResults(blockInstruction);
    auto* result = new CCompound();
    auto blockLabel = pushLabel(resultTypes);
    auto label = labelStack.back().label;
    auto stackSize = expressionStack.size();
    auto labelStackSize = labelStack.size();
    auto types = getBlockResults(blockInstruction);
    auto count = types.size();
    auto* resultNode = makeBlockResults(types);

    if (resultNode != nullptr) {
        result->addStatement(resultNode);
    }

    labelStack.back().branchTarget = true;
    labelStack.back().backward = true;

    result->addStatement(new CLabel(blockLabel));

    while (auto* statement = generateCStatement()) {
        result->addStatement(statement);
        if (labelStack.size() < labelStackSize) {
            break;
        }
    }

    for (auto i = count; expressionStack.size() > stackSize && i-- > 0; ) {
        auto* resultNameNode = new CNameUse(makeResultName(label, i));

        result->addStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);

        popLabel();
    }

    for (size_t i = 0; i < count; ++i) {
        pushExpression(new CNameUse(makeResultName(label, i)));
    }

    return result;
}

CNode* CGenerator::generateCIf(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultTypes = getBlockResults(blockInstruction);
    auto* condition = popExpression();
    auto types = getBlockResults(blockInstruction);
    auto* result = new CIf(condition, label, types);
    auto blockLabel = pushLabel(resultTypes);
    auto label = labelStack.back().label;
    auto stackSize = expressionStack.size();
    auto labelStackSize = labelStack.size();
    auto count = types.size();
    auto* resultNode = makeBlockResults(types);

    if (resultNode != nullptr) {
        result->setResultDeclaration(resultNode);
    }


    labelStack.back().impliedTarget = true;

    while (auto* statement = generateCStatement()) {
        result->addThenStatement(statement);
        if (labelStack.size() < labelStackSize) {
            break;
        }
    }

    for (auto i = count; expressionStack.size() > stackSize && i-- > 0; ) {
        auto* resultNameNode = new CNameUse(makeResultName(label, i));

        result->addThenStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
    }

    if (instructionPointer != instructionEnd && instructionPointer->get()->getOpcode() == Opcode::else_) {
        ++instructionPointer;
        while (auto* statement = generateCStatement()) {
            result->addElseStatement(statement);
            if (labelStack.size() < labelStackSize) {
                break;
            }
        }

        for (auto i = count; expressionStack.size() > stackSize && i-- > 0; ) {
            auto* resultNameNode = new CNameUse(makeResultName(label, i));

            result->addElseStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
        }
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        if (labelStack.back().branchTarget) {
            result->setLabelDeclaration(new CLabel(blockLabel));
        }

        popLabel();
    } else {
        result->setLabelDeclaration(new CLabel(blockLabel));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);
        if (!labelStack.back().branchTarget) {
            result->removeResultDeclaration();
        }

        popLabel();
    }

    for (size_t i = 0; i < count; ++i) {
        pushExpression(new CNameUse(makeResultName(label, i)));
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

    if (!labelInfo.backward && !labelInfo.types.empty()) {
        auto* compound = new CCompound;
        auto count = labelInfo.types.size();

        for (auto i = count; i-- > 0; ) {
            auto resultName = makeResultName(labelInfo.label, i);
            auto* result = popExpression();
            auto* assignment = new CBinaryExpression("=", new CNameUse(resultName), result);

            compound->addStatement(assignment);
        }

        compound->addStatement(new CBr(labelInfo.label));
        if (conditional) {
            for (unsigned i = 0; i < count; ++i) {
                pushExpression(new CNameUse(makeResultName(labelInfo.label, i)));
            }
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
    auto* condition = popExpression();
    auto* ifNode = new CIf(condition);

    ifNode->addThenStatement(generateCBranchStatement(index, true));
    return ifNode;
}

CNode* CGenerator::generateCBrUnless(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(instruction);
    auto index = branchInstruction->getIndex();
    auto* condition = notExpression(popExpression());
    auto* ifNode = new CIf(condition);

    ifNode->addThenStatement(generateCBranchStatement(index, true));
    return ifNode;
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

    return new CBinaryExpression(op, left, right);
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

    return new CBinaryExpression(op, new CCast(type, left), new CCast(type, right));
}

CNode* CGenerator::makeCombinedOffset(Instruction* instruction)
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
            combinedOffset = new CBinaryExpression("+", offsetEpression, new CI64(*value));
        }
    } else {
        combinedOffset = new CBinaryExpression("+", new CI64(offset), dynamicOffset);
    }

    return combinedOffset;
}

CNode* CGenerator::generateCLoad(std::string_view name, Instruction* instruction)
{
    auto* memoryInstruction = static_cast<InstructionMemory*>(instruction);
    auto* memory = module->getMemory(0);

    return new CLoad(name, memory->getCName(), makeCombinedOffset(instruction));
}

CNode* CGenerator::generateCLoadSplat(std::string_view splatName, std::string_view loadName, Instruction* instruction)
{
    auto* result = new CCall(splatName);

    result->addArgument(generateCLoad(loadName, instruction));

    return result;
}

CNode* CGenerator::generateCLoadExtend(std::string_view loadName, Instruction* instruction)
{
    auto* result = new CCall(loadName);

    result->addArgument(makeCombinedOffset(instruction));

    return result;
}

CNode* CGenerator::generateCStore(std::string_view name, Instruction* instruction)
{
    auto* valueToStore = popExpression();
    auto* memoryInstruction = static_cast<InstructionMemory*>(instruction);
    auto* memory = module->getMemory(0);
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
            combinedOffset = new CBinaryExpression("+", offsetEpression, new CI64(*value));
        }
    } else {
        combinedOffset = new CBinaryExpression("+", new CI64(offset), dynamicOffset);
    }

    return new CStore(name, memory->getCName(), combinedOffset, valueToStore);
}

CNode* CGenerator::generateCLocalSet(Instruction* instruction)
{
    auto* left = new CNameUse(localName(instruction));
    auto* right = popExpression();

    return new CBinaryExpression("=", left, right);
}

CNode* CGenerator::generateCGlobalSet(Instruction* instruction)
{
    auto* left = new CNameUse(globalName(instruction));
    auto* right = popExpression();

    return new CBinaryExpression("=", left, right);
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

CNode* CGenerator::generateCMemorySize()
{
    auto* memory = module->getMemory(0);
    auto* pageCount = new CBinaryExpression(".", new CNameUse(memory->getCName()),
            new CNameUse("pageCount"));

    return new CBinaryExpression("*",  pageCount, new CNameUse("memoryPageSize"));
}

CNode* CGenerator::generateCMemoryGrow()
{
    auto* memory = module->getMemory(0);
    auto* result = new CCall("growMemory");

    result->addArgument(new CUnaryExpression("&", new CNameUse(memory->getCName())));
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
    auto* result = new CCall(calledFunction->getCName());
    auto& results = signature->getResults();
    std::vector<std::string> temps;

    for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
        auto* argument = popExpression();

        result->addArgument(argument);
    }

    if (results.size() > 1) {
        temps = getTemps(results);

        for (auto i = results.size(); i-- > 0; ) {
            result->addArgument(new CUnaryExpression("&", new CNameUse(temps[i])));
        }
    }

    result->reverseArguments();

    if (results.empty()) {
        statement = result;
    } else if (results.size() == 1) {
        expression = result;
    } else {
        statement = result;
        for (const auto& temp : temps) {
            pushExpression(new CNameUse(temp));
        }
    }
}

void CGenerator::generateCCallIndirect(Instruction* instruction, CNode*& expression, CNode*& statement)
{
    auto callInstruction = static_cast<InstructionIndirect*>(instruction);
    auto typeIndex = callInstruction->getTypeIndex();
    auto* tableIndex = popExpression();
    auto* result = new CCallIndirect(typeIndex, tableIndex);
    auto* signature = module->getType(typeIndex)->getSignature();
    auto& results = signature->getResults();
    std::vector<std::string> temps;

    for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
        auto* argument = popExpression();

        result->addArgument(argument);
    }

    if (results.size() > 1) {
        temps = getTemps(results);

        for (auto i = results.size(); i-- > 0; ) {
            result->addArgument(new CUnaryExpression("&", new CNameUse(temps[i])));
        }
    }

    result->reverseArguments();

    if (results.empty()) {
        statement = result;
    } else if (results.size() == 1) {
        expression = result;
    } else {
        statement = result;
        for (const auto& temp : temps) {
            pushExpression(new CNameUse(temp));
        }
    }
}

CNode* CGenerator::generateCExtractLane(Instruction* instruction, const char* type)
{
    auto* laneInstruction = static_cast<InstructionIdx*>(instruction);
    auto index = laneInstruction->getIndex();
    auto* result = new CCall("v128ExtractLane"s + type);

    result->addArgument(popExpression());
    result->addArgument(new CI32(index));

    return result;
}

CNode* CGenerator::generateCReplaceLane(Instruction* instruction, const char* type)
{
    auto* laneInstruction = static_cast<InstructionIdx*>(instruction);
    auto index = laneInstruction->getIndex();
    auto* result = new CCall("v128ReplaceLane"s + type);
    auto* newValue = popExpression();

    result->addArgument(popExpression());
    result->addArgument(newValue);
    result->addArgument(new CI32(index));

    return result;
}

CNode* CGenerator::generateCShuffle(Instruction* instruction)
{
    auto* shuffleInstruction = static_cast<InstructionShuffle*>(instruction);
    const auto mask = shuffleInstruction->getValue();
    auto* result = new CCall("v128Shufflei8x16");

    auto* newValue = popExpression();

    result->addArgument(popExpression());
    result->addArgument(newValue);

    auto* makeCall = new CCall("v128Makei8x16");

    for (int i = 0; i < 16; ++i) {
        makeCall->addArgument(new CI32(mask[i]));
    }

    result->addArgument(makeCall);


    return result;
}

void CGenerator::generateCFunction()
{
    function = new CFunction(module->getFunction(codeEntry->getNumber())->getSignature());

    auto resultTypes = function->getSignature()->getResults();
    auto count = resultTypes.size();

    function->addStatement(tempNode = new CCompound);

    pushLabel(resultTypes);

    auto* resultNode = makeBlockResults(resultTypes);

    while (auto* statement = generateCStatement()) {
        function->addStatement(statement);
    }

    bool needsReturn = count != 0 && !expressionStack.empty();

    if (!labelStack.empty() && labelStack.back().branchTarget) {
        if (needsReturn) {
            for (auto i = count; i-- > 0; ) {
                auto* resultNameNode = new CNameUse(makeResultName(0, i));

                function->addStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
            }
        }

        function->addStatement(new CLabel(0));

        if (needsReturn) {
            if (count == 1) {
                function->addStatement(new CReturn(new CNameUse("result0")));
            } else {
                for (size_t i = 0; i < count; ++i) {
                    auto resultName = makeResultName(0, i);
                    auto* resultNameNode  = new CNameUse(resultName);
                    auto* resultPointerNode = new CNameUse('*' + resultName + "_ptr");

                    function->addStatement(new CBinaryExpression("=", resultPointerNode, resultNameNode));
                }
            }
        } else {
            delete resultNode;
        }
    } else if (needsReturn) {
        if (count == 1) {
            function->addStatement(new CReturn(popExpression()));
        } else {
            for (auto i = count; i-- > 0; ){
                auto resultName = makeResultName(0, i);
                auto* resultPointerNode = new CNameUse('*' + resultName + "_ptr");

                function->addStatement(new CBinaryExpression("=", resultPointerNode, popExpression()));
            }
        }

        delete resultNode;
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

            case Opcode::f32__copysign:
                expression = generateCCallPredef("copysignf", 2);
                break;

            case Opcode::f64__copysign:
                expression = generateCCallPredef("copysign", 2);
                break;


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
                expression = generateCMemorySize();
                break;

            case Opcode::memory__grow:
                expression = generateCMemoryGrow();
                break;

            case Opcode::i32__extend8_s:
                expression = generateCDoubleCast("int32_t", "int8_t");
                break;

            case Opcode::i32__extend16_s:
                expression = generateCDoubleCast("int32_t", "int16_t");
                break;

            case Opcode::i64__extend8_s:
                expression = generateCDoubleCast("int64_t", "int8_t");
                break;

            case Opcode::i64__extend16_s:
                expression = generateCDoubleCast("int64_t", "int16_t");
                break;

            case Opcode::i64__extend32_s:
                expression = generateCDoubleCast("int64_t", "int32_t");
                break;

    //      case Opcode::ref__null:
    //      case Opcode::ref__is_null:
    //      case Opcode::ref__func:
    //      case Opcode::alloca:

            case Opcode::br_unless:
                statement = generateCBrUnless(instruction);
                break;

    //      case Opcode::call_host:
    //      case Opcode::data:
    //      case Opcode::drop_keep:

            //EXTNS
            case Opcode::i32__trunc_sat_f32_s:
                expression = generateCCallPredef("satI32F32");
                break;

            case Opcode::i32__trunc_sat_f32_u:
                expression = generateCCallPredef("satU32F32");
                break;

            case Opcode::i32__trunc_sat_f64_s:
                expression = generateCCallPredef("satI32F64");
                break;

            case Opcode::i32__trunc_sat_f64_u:
                expression = generateCCallPredef("satU32F64");
                break;

            case Opcode::i64__trunc_sat_f32_s:
                expression = generateCCallPredef("satI64F32");
                break;

            case Opcode::i64__trunc_sat_f32_u:
                expression = generateCCallPredef("satU64F32");
                break;

            case Opcode::i64__trunc_sat_f64_s:
                expression = generateCCallPredef("satI64F64");
                break;

            case Opcode::i64__trunc_sat_f64_u:
                expression = generateCCallPredef("satU64F64");
                break;

    //      case Opcode::memory__init:
    //      case Opcode::data__drop:
    //      case Opcode::memory__copy:
    //      case Opcode::memory__fill:
    //      case Opcode::table__init:
    //      case Opcode::elem__drop:
    //      case Opcode::table__copy:
    //      case Opcode::table__grow:
    //      case Opcode::table__size:
    //      case Opcode::table__fill:

            // SIMD
            case Opcode::v128__load:
                expression = generateCLoad("loadV128", instruction);
                break;

            case Opcode::i16x8__load8x8_s:
                expression = generateCLoadExtend("v128SLoadExti16x8", instruction);
                break;

            case Opcode::i16x8__load8x8_u:
                expression = generateCLoadExtend("v128SLoadExtu16x8", instruction);
                break;

            case Opcode::i32x4__load16x4_s:
                expression = generateCLoadExtend("v128SLoadExti32x4", instruction);
                break;

            case Opcode::i32x4__load16x4_u:
                expression = generateCLoadExtend("v128SLoadExtu32x4", instruction);
                break;

            case Opcode::i64x2__load32x2_s:
                expression = generateCLoadExtend("v128SLoadExti64x2", instruction);
                break;

            case Opcode::i64x2__load32x2_u:
                expression = generateCLoadExtend("v128SLoadExtu64x2", instruction);
                break;

            case Opcode::v8x16__load_splat:
                expression = generateCLoadSplat("v128Splati8x16", "loadI32", instruction);
                break;

            case Opcode::v16x8__load_splat:
                expression = generateCLoadSplat("v128Splati16x8", "loadI32", instruction);
                break;

            case Opcode::v32x4__load_splat:
                expression = generateCLoadSplat("v128Splati32x4", "loadI32", instruction);
                break;

            case Opcode::v64x2__load_splat:
                expression = generateCLoadSplat("v128Splati64x2", "loadI64", instruction);
                break;

            case Opcode::v128__store:
                expression = generateCStore("storeV128", instruction);
                break;

            case Opcode::v128__const:
                expression = new CV128(static_cast<InstructionV128*>(instruction)->getValue());
                break;

            case Opcode::v8x16__shuffle:
                expression = generateCShuffle(instruction);
                break;

            case Opcode::v8x16__swizzle:
                expression = generateCCallPredef("v128Swizzlei8x16", 2);
                break;

            case Opcode::i8x16__splat:
                expression = generateCCallPredef("v128Splati8x16");
                break;

            case Opcode::i16x8__splat:
                expression = generateCCallPredef("v128Splati16x8");
                break;

            case Opcode::i32x4__splat:
                expression = generateCCallPredef("v128Splati32x4");
                break;

            case Opcode::i64x2__splat:
                expression = generateCCallPredef("v128Splati64x2");
                break;

            case Opcode::f32x4__splat:
                expression = generateCCallPredef("v128Splatf32x4");
                break;

            case Opcode::f64x2__splat:
                expression = generateCCallPredef("v128Splatf64x2");
                break;

            case Opcode::i8x16__extract_lane_s:
                expression = generateCExtractLane(instruction, "i8x16");
                break;

            case Opcode::i8x16__extract_lane_u:
                expression = generateCExtractLane(instruction, "i8x16");
                break;

            case Opcode::i8x16__replace_lane:
                expression = generateCReplaceLane(instruction, "i8x16");
                break;

            case Opcode::i16x8__extract_lane_s:
                expression = generateCExtractLane(instruction, "i16x8");
                break;

            case Opcode::i16x8__extract_lane_u:
                expression = generateCExtractLane(instruction, "u16x8");
                break;

            case Opcode::i16x8__replace_lane:
                expression = generateCReplaceLane(instruction, "i16x8");
                break;

            case Opcode::i32x4__extract_lane:
                expression = generateCExtractLane(instruction, "i32x4");
                break;

            case Opcode::i32x4__replace_lane:
                expression = generateCReplaceLane(instruction, "i32x4");
                break;

            case Opcode::i64x2__extract_lane:
                expression = generateCExtractLane(instruction, "i64x2");
                break;

            case Opcode::i64x2__replace_lane:
                expression = generateCReplaceLane(instruction, "i64x2");
                break;

            case Opcode::f32x4__extract_lane:
                expression = generateCExtractLane(instruction, "f32x4");
                break;

            case Opcode::f32x4__replace_lane:
                expression = generateCReplaceLane(instruction, "f32x4");
                break;

            case Opcode::f64x2__extract_lane:
                expression = generateCExtractLane(instruction, "f64x2");
                break;

            case Opcode::f64x2__replace_lane:
                expression = generateCReplaceLane(instruction, "f64x2");
                break;

            case Opcode::i8x16__eq:
                expression = generateCCallPredef("v128Eqi8x16", 2);
                break;

            case Opcode::i8x16__ne:
                expression = generateCCallPredef("v128Nei8x16", 2);
                break;

            case Opcode::i8x16__lt_s:
                expression = generateCCallPredef("v128Lti8x16", 2);
                break;

            case Opcode::i8x16__lt_u:
                expression = generateCCallPredef("v128Ltu8x16", 2);
                break;

            case Opcode::i8x16__gt_s:
                expression = generateCCallPredef("v128Gti8x16", 2);
                break;

            case Opcode::i8x16__gt_u:
                expression = generateCCallPredef("v128Gtu8x16", 2);
                break;

            case Opcode::i8x16__le_s:
                expression = generateCCallPredef("v128Lei8x16", 2);
                break;

            case Opcode::i8x16__le_u:
                expression = generateCCallPredef("v128Leu8x16", 2);
                break;

            case Opcode::i8x16__ge_s:
                expression = generateCCallPredef("v128Gei8x16", 2);
                break;

            case Opcode::i8x16__ge_u:
                expression = generateCCallPredef("v128Geu8x16", 2);
                break;

            case Opcode::i16x8__eq:
                expression = generateCCallPredef("v128Eqi16x8", 2);
                break;

            case Opcode::i16x8__ne:
                expression = generateCCallPredef("v128Nei16x8", 2);
                break;

            case Opcode::i16x8__lt_s:
                expression = generateCCallPredef("v128Lti16x8", 2);
                break;

            case Opcode::i16x8__lt_u:
                expression = generateCCallPredef("v128Ltu16x8", 2);
                break;

            case Opcode::i16x8__gt_s:
                expression = generateCCallPredef("v128Gti16x8", 2);
                break;

            case Opcode::i16x8__gt_u:
                expression = generateCCallPredef("v128Gtu16x8", 2);
                break;

            case Opcode::i16x8__le_s:
                expression = generateCCallPredef("v128Lei16x8", 2);
                break;

            case Opcode::i16x8__le_u:
                expression = generateCCallPredef("v128Leu16x8", 2);
                break;

            case Opcode::i16x8__ge_s:
                expression = generateCCallPredef("v128Gei16x8", 2);
                break;

            case Opcode::i16x8__ge_u:
                expression = generateCCallPredef("v128Geu16x8", 2);
                break;

            case Opcode::i32x4__eq:
                expression = generateCCallPredef("v128Eqi32x4", 2);
                break;

            case Opcode::i32x4__ne:
                expression = generateCCallPredef("v128Nei32x4", 2);
                break;

            case Opcode::i32x4__lt_s:
                expression = generateCCallPredef("v128Lti32x4", 2);
                break;

            case Opcode::i32x4__lt_u:
                expression = generateCCallPredef("v128Ltu32x4", 2);
                break;

            case Opcode::i32x4__gt_s:
                expression = generateCCallPredef("v128Gti32x4", 2);
                break;

            case Opcode::i32x4__gt_u:
                expression = generateCCallPredef("v128Gtu32x4", 2);
                break;

            case Opcode::i32x4__le_s:
                expression = generateCCallPredef("v128Lei32x4", 2);
                break;

            case Opcode::i32x4__le_u:
                expression = generateCCallPredef("v128Leu32x4", 2);
                break;

            case Opcode::i32x4__ge_s:
                expression = generateCCallPredef("v128Gei32x4", 2);
                break;

            case Opcode::i32x4__ge_u:
                expression = generateCCallPredef("v128Geu32x4", 2);
                break;

            case Opcode::f32x4__eq:
                expression = generateCCallPredef("v128Eqf32x4", 2);
                break;

            case Opcode::f32x4__ne:
                expression = generateCCallPredef("v128Nef42x4", 2);
                break;

            case Opcode::f32x4__lt:
                expression = generateCCallPredef("v128Ltf42x4", 2);
                break;

            case Opcode::f32x4__gt:
                expression = generateCCallPredef("v128Gtf42x4", 2);
                break;

            case Opcode::f32x4__le:
                expression = generateCCallPredef("v128Lef42x4", 2);
                break;

            case Opcode::f32x4__ge:
                expression = generateCCallPredef("v128Gef42x4", 2);
                break;

            case Opcode::f64x2__eq:
                expression = generateCCallPredef("v128Eqf64x2", 2);
                break;

            case Opcode::f64x2__ne:
                expression = generateCCallPredef("v128Nef64x2", 2);
                break;

            case Opcode::f64x2__lt:
                expression = generateCCallPredef("v128Ltf64x2", 2);
                break;

            case Opcode::f64x2__gt:
                expression = generateCCallPredef("v128Gtf64x2", 2);
                break;

            case Opcode::f64x2__le:
                expression = generateCCallPredef("v128Lef64x2", 2);
                break;

            case Opcode::f64x2__ge:
                expression = generateCCallPredef("v128Gef64x2", 2);
                break;

            case Opcode::v128__not:
                expression = generateCCallPredef("v128Noti64x2");
                break;

            case Opcode::v128__and:
                expression = generateCCallPredef("v128Andi64x2", 2);
                break;

            case Opcode::v128__andnot:
                expression = generateCCallPredef("v128AndNoti64x2", 2);
                break;

            case Opcode::v128__or:
                expression = generateCCallPredef("v128Ori64x2", 2);
                break;

            case Opcode::v128__xor:
                expression = generateCCallPredef("v128Xori64x2", 2);
                break;

            case Opcode::v128__bitselect:
                expression = generateCCallPredef("v128Bitselect", 3);
                break;

            case Opcode::i8x16__abs:
                expression = generateCCallPredef("v128Absi8x16");
                break;

            case Opcode::i8x16__neg:
                expression = generateCCallPredef("v128Negi8x16");
                break;

            case Opcode::i8x16__any_true:
                expression = generateCCallPredef("v128SAnyTruei8x16", 2);
                break;

            case Opcode::i8x16__all_true:
                expression = generateCCallPredef("v128SAllTruei8x16", 2);
                break;

            case Opcode::i8x16__narrow_i16x8_s:
                expression = generateCCallPredef("narrowI8x16I16x8", 2);
                break;

            case Opcode::i8x16__narrow_i16x8_u:
                expression = generateCCallPredef("narrowU8x16I16x8", 2);
                break;

            case Opcode::i8x16__shl:
                expression = generateCCallPredef("v128Shli8x16", 2);
                break;

            case Opcode::i8x16__shr_s:
                expression = generateCCallPredef("v128Shri8x16", 2);
                break;

            case Opcode::i8x16__shr_u:
                expression = generateCCallPredef("v128Shru8x16", 2);
                break;

            case Opcode::i8x16__add:
                expression = generateCCallPredef("v128Addi8x16", 2);
                break;

            case Opcode::i8x16__add_saturate_s:
                expression = generateCCallPredef("v128SatAddi8x16", 2);
                break;

            case Opcode::i8x16__add_saturate_u:
                expression = generateCCallPredef("v128SatAddu8x16", 2);
                break;

            case Opcode::i8x16__sub:
                expression = generateCCallPredef("v128Subi8x16", 2);
                break;

            case Opcode::i8x16__sub_saturate_s:
                expression = generateCCallPredef("v128SatSubi8x16", 2);
                break;

            case Opcode::i8x16__sub_saturate_u:
                expression = generateCCallPredef("v128SatSubu8x16", 2);
                break;

            case Opcode::i8x16__min_s:
                expression = generateCCallPredef("v128Mini8x16", 2);
                break;

            case Opcode::i8x16__min_u:
                expression = generateCCallPredef("v128Minu8x16", 2);
                break;

            case Opcode::i8x16__max_s:
                expression = generateCCallPredef("v128Maxi8x16", 2);
                break;

            case Opcode::i8x16__max_u:
                expression = generateCCallPredef("v128Maxu8x16", 2);
                break;

            case Opcode::i8x16__avgr_u:
                expression = generateCCallPredef("v128Avgru8x16", 2);
                break;

            case Opcode::i16x8__abs:
                expression = generateCCallPredef("v128Absi8x16");
                break;

            case Opcode::i16x8__neg:
                expression = generateCCallPredef("v128Negi8x16");
                break;

            case Opcode::i16x8__any_true:
                expression = generateCCallPredef("v128SAnyTruei16x8", 2);
                break;

            case Opcode::i16x8__all_true:
                expression = generateCCallPredef("v128SAllTruei16x8", 2);
                break;

            case Opcode::i16x8__narrow_i32x4_s:
                expression = generateCCallPredef("narrowI16x8I32x4", 2);
                break;

            case Opcode::i16x8__narrow_i32x4_u:
                expression = generateCCallPredef("narrowU16x8I32x4", 2);
                break;

            case Opcode::i16x8__widen_low_i8x16_s:
                expression = generateCCallPredef("v128WidenLowi16x8i8x16");
                break;

            case Opcode::i16x8__widen_high_i8x16_s:
                expression = generateCCallPredef("v128WidenHighi16x8i8x16");
                break;

            case Opcode::i16x8__widen_low_i8x16_u:
                expression = generateCCallPredef("v128WidenLowi16x8u8x16");
                break;

            case Opcode::i16x8__widen_high_i8x16_u:
                expression = generateCCallPredef("v128WidenHighi16x8u8x16");
                break;

            case Opcode::i16x8__shl:
                expression = generateCCallPredef("v128Shli16x8", 2);
                break;

            case Opcode::i16x8__shr_s:
                expression = generateCCallPredef("v128Shri16x8", 2);
                break;

            case Opcode::i16x8__shr_u:
                expression = generateCCallPredef("v128Shru16x8", 2);
                break;

            case Opcode::i16x8__add:
                expression = generateCCallPredef("v128Addi16x8", 2);
                break;

            case Opcode::i16x8__add_saturate_s:
                expression = generateCCallPredef("v128SatAddi16x8", 2);
                break;

            case Opcode::i16x8__add_saturate_u:
                expression = generateCCallPredef("v128SatAddu16x8", 2);
                break;

            case Opcode::i16x8__sub:
                expression = generateCCallPredef("v128Subi16x8", 2);
                break;

            case Opcode::i16x8__sub_saturate_s:
                expression = generateCCallPredef("v128SatSubi16x8", 2);
                break;

            case Opcode::i16x8__sub_saturate_u:
                expression = generateCCallPredef("v128SatSubu16x8", 2);
                break;

            case Opcode::i16x8__mul:
                expression = generateCCallPredef("v128Muli16x8", 2);
                break;

            case Opcode::i16x8__min_s:
                expression = generateCCallPredef("v128Mini16x8", 2);
                break;

            case Opcode::i16x8__min_u:
                expression = generateCCallPredef("v128Mini16x8", 2);
                break;

            case Opcode::i16x8__max_s:
                expression = generateCCallPredef("v128Maxi16x8", 2);
                break;

            case Opcode::i16x8__max_u:
                expression = generateCCallPredef("v128Maxu16x8", 2);
                break;

            case Opcode::i16x8__avgr_u:
                expression = generateCCallPredef("v128Avgru16x8", 2);
                break;

            case Opcode::i32x4__abs:
                expression = generateCCallPredef("v128Absi16x8");
                break;

            case Opcode::i32x4__neg:
                expression = generateCCallPredef("v128Negi16x8");
                break;

            case Opcode::i32x4__any_true:
                expression = generateCCallPredef("v128SAnyTruei32x4", 2);
                break;

            case Opcode::i32x4__all_true:
                expression = generateCCallPredef("v128SAllTruei32x4", 2);
                break;

            case Opcode::i32x4__widen_low_i16x8_s:
                expression = generateCCallPredef("v128WidenLowi32x4i16x8");
                break;

            case Opcode::i32x4__widen_high_i16x8_s:
                expression = generateCCallPredef("v128WidenHighi32x4i16x8");
                break;

            case Opcode::i32x4__widen_low_i16x8_u:
                expression = generateCCallPredef("v128WidenLowi32x4u16x8");
                break;

            case Opcode::i32x4__widen_high_i16x8_u:
                expression = generateCCallPredef("v128WidenHighi32x4u16x8");
                break;

            case Opcode::i32x4__shl:
                expression = generateCCallPredef("v128Shli32x4", 2);
                break;

            case Opcode::i32x4__shr_s:
                expression = generateCCallPredef("v128Shri32x4", 2);
                break;

            case Opcode::i32x4__shr_u:
                expression = generateCCallPredef("v128Shru32x4", 2);
                break;

            case Opcode::i32x4__add:
                expression = generateCCallPredef("v128Addi32x4", 2);
                break;

            case Opcode::i32x4__sub:
                expression = generateCCallPredef("v128Subi32x4", 2);
                break;

            case Opcode::i32x4__mul:
                expression = generateCCallPredef("v128Muli32x4", 2);
                break;

            case Opcode::i32x4__min_s:
                expression = generateCCallPredef("v128Mini32x4", 2);
                break;

            case Opcode::i32x4__min_u:
                expression = generateCCallPredef("v128Mini32x4", 2);
                break;

            case Opcode::i32x4__max_s:
                expression = generateCCallPredef("v128Maxi32x4", 2);
                break;

            case Opcode::i32x4__max_u:
                expression = generateCCallPredef("v128Maxu32x4", 2);
                break;

            case Opcode::i64x2__neg:
                expression = generateCCallPredef("v128Negi32x4");
                break;

            case Opcode::i64x2__shl:
                expression = generateCCallPredef("v128Shli64x2", 2);
                break;

            case Opcode::i64x2__shr_s:
                expression = generateCCallPredef("v128Shri64x2", 2);
                break;

            case Opcode::i64x2__shr_u:
                expression = generateCCallPredef("v128Shru64x2", 2);
                break;

            case Opcode::i64x2__add:
                expression = generateCCallPredef("v128Addi64x2", 2);
                break;

            case Opcode::i64x2__sub:
                expression = generateCCallPredef("v128Subi64x2", 2);
                break;

            case Opcode::i64x2__mul:
                expression = generateCCallPredef("v128Muli64x2", 2);
                break;

            case Opcode::f32x4__abs:
                expression = generateCCallPredef("v128Absf32x4");
                break;

            case Opcode::f32x4__neg:
                expression = generateCCallPredef("v128Negf32x4");
                break;

            case Opcode::f32x4__sqrt:
                expression = generateCCallPredef("v128Sqrtf32x4");
                break;

            case Opcode::f32x4__add:
                expression = generateCCallPredef("v128Addf32x4", 2);
                break;

            case Opcode::f32x4__sub:
                expression = generateCCallPredef("v128Subf32x4", 2);
                break;

            case Opcode::f32x4__mul:
                expression = generateCCallPredef("v128Mulf32x4", 2);
                break;

            case Opcode::f32x4__div:
                expression = generateCCallPredef("v128Divf32x4", 2);
                break;

            case Opcode::f32x4__min:
                expression = generateCCallPredef("v128Minf32x4", 2);
                break;

            case Opcode::f32x4__max:
                expression = generateCCallPredef("v128Maxf32x4", 2);
                break;

            case Opcode::f64x2__abs:
                expression = generateCCallPredef("v128Absf64x2");
                break;

            case Opcode::f64x2__neg:
                expression = generateCCallPredef("v128Negf64x2");
                break;

            case Opcode::f64x2__sqrt:
                expression = generateCCallPredef("v128Sqrtf64x2");
                break;

            case Opcode::f64x2__add:
                expression = generateCCallPredef("v128Addf64x2", 2);
                break;

            case Opcode::f64x2__sub:
                expression = generateCCallPredef("v128Subf64x2", 2);
                break;

            case Opcode::f64x2__mul:
                expression = generateCCallPredef("v128Mulf64x2", 2);
                break;

            case Opcode::f64x2__div:
                expression = generateCCallPredef("v128Divf64x2", 2);
                break;

            case Opcode::f64x2__min:
                expression = generateCCallPredef("v128Minf64x2", 2);
                break;

            case Opcode::f64x2__max:
                expression = generateCCallPredef("v128Maxf64x2", 2);
                break;

            case Opcode::i32x4__trunc_sat_f32x4_s:
                expression = generateCCallPredef("satI32x4F32x4", 1);
                break;

            case Opcode::i32x4__trunc_sat_f32x4_u:
                expression = generateCCallPredef("satU32x4F32x4", 1);
                break;

            case Opcode::f32x4__convert_i32x4_s:
                expression = generateCCallPredef("convertF32x4I32x4", 1);
                break;

            case Opcode::f32x4__convert_i32x4_u:
                expression = generateCCallPredef("convertF32x4U32x4", 1);
                break;

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

    statement->generateC(os, *this);

    if (kind != CNode::kIf && kind != CNode::kSwitch &&
            kind != CNode::kCompound && kind != CNode::kLabel) {
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

void CGenerator::optimize()
{
    traverse(function, [&](CNode* node) {
            if (auto* labelstatement = node->castTo<CLabel>(); labelstatement != nullptr) {
                labelMap[labelstatement->getLabel()].declaration = node;
            } else if (auto* branchStatement = node->castTo<CBr>(); branchStatement != nullptr) {
                labelMap[branchStatement->getLabel()].useCount++;
            }
        });

    function->optimize(*this);
}

void CGenerator::generateC(std::ostream& os)
{
    if (optimized) {
        optimize();
    }

    function->generateC(os, *this);
}

CGenerator::CGenerator(Module* module, CodeEntry* codeEntry, bool optimized)
  : module(module), codeEntry(codeEntry), optimized(optimized)
{
    buildCTree();
}

void CGenerator::decrementUseCount(unsigned label)
{
    auto it = labelMap.find(label);

    if (it != labelMap.end()) {
        if (--it->second.useCount == 0) {
            delete it->second.declaration;
            labelMap.erase(it);
        }
    }
}

};
