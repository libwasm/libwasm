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
#include <tuple>
#include <optional>

using namespace std::string_literals;

namespace libwasm
{

static bool equalNodes(CNode* node1, CNode* node2)
{
    if (node1 == nullptr) {
        return node2 == nullptr;
    }

    if (node2 == nullptr) {
        return false;
    }

    return node1->equals(node2);
}

template<typename T>
bool equalVectors(std::vector<T*>& vector1, std::vector<T*>& vector2)
{
    if (vector1.size() != vector2.size()) {
        return false;
    }

    for (size_t i = 0, c = vector1.size(); i < c; ++i) {
        if (!equalNodes(vector1[i], vector2[i])) {
            return false;
        }
    }

    return true;
}

static void traverse(CNode* node, std::function<void(CNode*)> exec)
{
    for (auto* next = node->getChild(); next != nullptr; next = next->traverseToNext(node)) {
        exec(next);
    }
}

static void traverseStatements(CCompound* node, std::function<void(CNode*)> exec)
{
    for (auto* next = node->getChild(); next != nullptr; ) {
        exec(next);

        if (auto* ifStatement = next->castTo<CIf>(); ifStatement != nullptr) {
            traverseStatements(ifStatement->getThenStatements(), exec);
            traverseStatements(ifStatement->getElseStatements(), exec);
            if (ifStatement->getCondition() == nullptr) {
                ifStatement->getThenStatements()->link(node, ifStatement);
                ifStatement->setNopped(true);
            }
        } else if (auto* switchStatement = next->castTo<CSwitch>(); switchStatement != nullptr) {
            for (auto& cs : switchStatement->getCases()) {
                traverseStatements(cs->statements, exec);
            }

            traverseStatements(switchStatement->getDefault(), exec);
        } else if (auto* loop = next->castTo<CLoop>(); loop != nullptr) {
            traverseStatements(loop->getBody(), exec);
        } else if (auto* compound = next->castTo<CCompound>(); compound != nullptr) {
            traverseStatements(compound, exec);
        }

        auto* node = next;

        next = next->getNext();

        if (node->isNopped()) {
            delete node;
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
            assert(false);
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
            assert(false);
            exit(1);
        }
    }

    if (p == nullptr) {
        p = n->parent;
    } else if (p != n->parent) {
        std::cerr << "Attempt to link a node in 2 chains" << std::endl;
        assert(false);
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

bool CNode::contains(CNode* node)
{
    for (auto* next = child; next != nullptr; next = next->traverseToNext(this)) {
        if (next->equals(node)) {
            return true;
        }
    }

    return false;
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
    generator.generateStatement(os, statements);
    generator.undent();
}

void CFunction::enhance(CGenerator& generator)
{
    traverse(this, [](CNode* node) {
            if (auto* expression = node->castTo<CBinaryExpression>(); expression != nullptr) {
                expression->enhance();
            }
        });

    statements->enhance(generator);
    statements->flatten();
    statements->enhanceVariables(generator);
}

bool CLabel::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherLabel = static_cast<CLabel*>(other);

    return label == otherLabel->getLabel();
}

void CLabel::generateC(std::ostream& os, CGenerator& generator)
{
    os << "\nlabel" << toString(label) << ":;";
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

        if (precedence == 2 || precedence == 11) {
            return true;
        }

        auto* binaryParent = static_cast<CBinaryExpression*>(parent);
        std::string_view parentOp = binaryParent->getOp();

        if (parentOp == op && (op == "+" || op == "*")) {
            return false;
        }

        auto parentPrecedence = binaryPrecedence(parentOp);

        if (parentPrecedence == 1) {
            return true;
        }

        if (parentPrecedence < 9) {
            return parentPrecedence != 2;
        }

        return parentPrecedence >= precedence;
    }

    if (parentKind == CNode::kUnaryExpression) {
        return true;
    }

    if (parentKind == CNode::kPostfixExpression) {
        return true;
    }

    if (parentKind == CNode::kTernaryExpression) {
        return true;
    }

    return false;
}

bool CBinaryExpression::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherBinary = static_cast<CBinaryExpression*>(other);

    return op == otherBinary->getOp() &&
        equalNodes(left, otherBinary->getLeft()) &&
        equalNodes(right, otherBinary->getRight());
}

void CBinaryExpression::enhance()
{
    if (op == "+") {
        if (auto* i32 = right->castTo<CI32>(); i32 != nullptr) {
            if (auto value = i32->getValue(); value < 0) {
                op = "-";
                i32->setValue(-value);
            }
        } else if (auto* i64 = right->castTo<CI64>(); i64 != nullptr) {
            if (auto value = i64->getValue(); value < 0) {
                op = "-";
                i64->setValue(-value);
            }
        } else if (auto* f32 = right->castTo<CF32>(); f32 != nullptr) {
            if (auto value = f32->getValue(); value < 0) {
                op = "-";
                f32->setValue(-value);
            }
        } else if (auto* f64 = right->castTo<CF64>(); f64 != nullptr) {
            if (auto value = f64->getValue(); value < 0) {
                op = "-";
                f64->setValue(-value);
            }
        }
    }
}

void CBinaryExpression::enhanceAssignment()
{
    if (op == "=") {
        if (left->getKind() == CNode::kNameUse && right->getKind() == CNode::kBinauryExpression) {
            auto* rightBinary = static_cast<CBinaryExpression*>(right);
            auto rightPrecedence = binaryPrecedence(rightBinary->getOp());

            if (rightPrecedence >= 13 || (rightPrecedence >= 6 && rightPrecedence <= 8)) {
                auto* rightLeft = rightBinary->left;
                auto* rightRight = rightBinary->right;

                if (rightLeft->getKind() == CNode::kNameUse) {
                    auto leftName = static_cast<CNameUse*>(left)->getName();
                    auto rightLeftName = static_cast<CNameUse*>(rightLeft)->getName();

                    if (leftName == rightLeftName) {
                        if (rightBinary->op == "+" || rightBinary->op == "-") {
                            bool isOne = false;

                            if (auto* i32 = rightRight->castTo<CI32>();
                                    i32 != nullptr && i32->getValue() == 1) {
                                isOne = true;
                            }

                            if (auto* i64 = rightRight->castTo<CI64>();
                                    i64 != nullptr && i64->getValue() == 1) {
                                isOne = true;
                            }

                            if (isOne) {
                                // a = a + 1 --> a++
                                const char* postOp = rightBinary->op == "+" ? "++" : "--";
                                auto* postfix = new CPostfixExpression(postOp, left);

                                postfix->link(parent, next);
                                nopped = true;
                                return;
                            }
                        }

                        // a = a + b --> a += b
                        op = rightBinary->op + "=";


                        rightBinary->unlink();
                        right = rightRight;
                        right->link(this);
                        delete rightBinary;

                        return;
                    }
                }
            }
        } else if (left->getKind() == CNode::kNameUse && right->getKind() == CNode::kNameUse) {
            auto leftName = static_cast<CNameUse*>(left)->getName();
            auto rightName = static_cast<CNameUse*>(right)->getName();

            if (leftName == rightName) {
                // a = a --> nop
                nopped = true;
                return;
            }
        }
    }
}

void CBinaryExpression::generateC(std::ostream& os, CGenerator& generator)
{
    bool parenthesis = needsParenthesis(this, op);

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

bool CI32::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherI32 = static_cast<CI32*>(other);

    return value == otherI32->getValue();
}

void CI32::generateC(std::ostream& os, CGenerator& generator)
{
    os << value;
}

bool CI64::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherI64 = static_cast<CI64*>(other);

    return value == otherI64->getValue();
}

void CI64::generateC(std::ostream& os, CGenerator& generator)
{
    if (uint64_t(value) == 0x8000000000000000ULL) {
        os << "0x8000000000000000ULL";
    } else {
        os << value << "LL";
    }
}

bool CNameUse::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherNameUse = static_cast<CNameUse*>(other);

    return name == otherNameUse->getName();
}

void CNameUse::generateC(std::ostream& os, CGenerator& generator)
{
    os << name;
}

bool CVariable::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherVariable = static_cast<CVariable*>(other);

    return type == otherVariable->getType() &&
        name == otherVariable->getName() &&
        equalNodes(initialValue, otherVariable->getInitialValue());
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

bool CLoad::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherLoad = static_cast<CLoad*>(other);

    return name == otherLoad->getName() &&
        memory == otherLoad->getMemory() &&
        equalNodes(offset, otherLoad->getOffset());
}

void CLoad::generateC(std::ostream& os, CGenerator& generator)
{
    os << name << "(&" << memory << ", ";
    offset->generateC(os, generator);
    os << ')';
}

bool CBranch::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    return label == static_cast<CBranch*>(other)->getLabel();
}

void CBranch::generateC(std::ostream& os, CGenerator& generator)
{
    os << "goto label" << label;
}

void CBreak::generateC(std::ostream& os, CGenerator& generator)
{
    os << "break";
}

void CContinue::generateC(std::ostream& os, CGenerator& generator)
{
    os << "continue";
}

bool CSubscript::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherSubscript = static_cast<CSubscript*>(other);

    return equalNodes(array, otherSubscript->getArray()) &&
        equalNodes(subscript, otherSubscript->getSubscript());
}

void CSubscript::generateC(std::ostream& os, CGenerator& generator)
{
    array->generateC(os, generator);
    os << '[';
    subscript->generateC(os, generator);
    os << ']';
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

bool CCallIndirect::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherCall = static_cast<CCallIndirect*>(other);

    return typeIndex == otherCall->getTypeIndex() &&
        tableIndex == otherCall->getTableIndex() &&
        equalNodes(indexInTable, otherCall->getIndexInTable()) &&
        equalVectors(arguments, otherCall->getArguments());
}

void CCallIndirect::generateC(std::ostream& os, CGenerator& generator)
{
    auto* table = generator.getModule()->getTable(tableIndex);

    os << "((" << generator.getModule()->getNamePrefix() << "type" << typeIndex << ')' <<
        table->getCName(generator.getModule()) << ".data[";
    indexInTable->generateC(os, generator);

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

bool CCall::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherCall = static_cast<CCall*>(other);

    return functionName == otherCall->getFunctionName() &&
        equalVectors(arguments, otherCall->getArguments());
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

bool CCast::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherCast = static_cast<CCast*>(other);

    return type == otherCast->getType() &&
        equalNodes(operand, otherCast->getOperand());
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

bool CF32::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherF32 = static_cast<CF32*>(other);

    return value == otherF32->getValue();
}

void CF32::generateC(std::ostream& os, CGenerator& generator)
{
    os << toString(value, true);
}

bool CF64::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherF64 = static_cast<CF64*>(other);

    return value == otherF64->getValue();
}

void CF64::generateC(std::ostream& os, CGenerator& generator)
{
    os << toString(value, true);
}

bool CV128::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherV128 = static_cast<CV128*>(other);

    return value == otherV128->getValue();
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
                    "LL, 0x" << std::setw(16) << std::setfill('0') << a64[1] << std::dec << "LL)";
}

bool CReturn::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherReturn = static_cast<CReturn*>(other);

    return equalNodes(value, otherReturn->getValue());
}

void CReturn::generateC(std::ostream& os, CGenerator& generator)
{
    os << "return";
    if (value != nullptr) {
        os << ' ';
        value->generateC(os, generator);
    }
}

bool CStore::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherStore = static_cast<CStore*>(other);

    return name == otherStore->getName() &&
        memory == otherStore->getMemory() &&
        equalNodes(offset, otherStore->getOffset()) &&
        equalNodes(value, otherStore->getValue());
}

void CStore::generateC(std::ostream& os, CGenerator& generator)
{
    os << name << "(&" << memory << ", ";
    offset->generateC(os, generator);
    os << ", ";
    value->generateC(os, generator);
    os << ')';
}

bool CTernaryExpression::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherTernaryExpression = static_cast<CTernaryExpression*>(other);

    return equalNodes(condition, otherTernaryExpression->getCondition()) &&
        equalNodes(trueExpression, otherTernaryExpression->getTrueExpression()) &&
        equalNodes(falseExpression, otherTernaryExpression->getFalseExpression());
}

void CTernaryExpression::generateC(std::ostream& os, CGenerator& generator)
{
    bool needsParenthesis = false;

    if (parent != nullptr) {
        auto parentKind = parent->getKind();

        if (parentKind == CNode::kCast || parentKind == CNode::kUnaryExpression ||
                parentKind == CNode::kPostfixExpression ||
                parentKind == CNode::kBinauryExpression || parentKind == CNode::kTernaryExpression) {
            needsParenthesis = true;
        }
    }

    if (needsParenthesis) {
        os << '(';
    }

    condition->generateC(os, generator);
    os << " ? ";
    trueExpression->generateC(os, generator);
    os << " : ";
    falseExpression->generateC(os, generator);

    if (needsParenthesis) {
        os << ')';
    }

}

bool CUnaryExpression::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherUnaryExpression = static_cast<CUnaryExpression*>(other);

    return op == otherUnaryExpression->getOp() &&
        equalNodes(operand, otherUnaryExpression->getOperand());
}

void CUnaryExpression::generateC(std::ostream& os, CGenerator& generator)
{
    os << op;
    operand->generateC(os, generator);
}

bool CPostfixExpression::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherPostfixExpression = static_cast<CPostfixExpression*>(other);

    return op == otherPostfixExpression->getOp() &&
        equalNodes(operand, otherPostfixExpression->getOperand());
}

void CPostfixExpression::generateC(std::ostream& os, CGenerator& generator)
{
    operand->generateC(os, generator);
    os << op;
}

void CCompound::addStatement(CNode* statement)
{
    statement->link(this);
}

bool CCompound::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* myStatement = getChild();
    auto* otherStatement = other->getChild();

    while (myStatement != nullptr) {
        if (otherStatement == nullptr || !myStatement->equals(otherStatement)) {
            return false;
        }

        myStatement = myStatement->getNext();
        otherStatement = otherStatement->getNext();
    }

    return otherStatement == nullptr;
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
        } else if (auto* switchStatement = statement->castTo<CSwitch>(); switchStatement != nullptr) {
            statement = statement->getNext();

            for (auto& cs : switchStatement->getCases()) {
                cs->statements->flatten();
            }

            switchStatement->getDefault()->flatten();
        } else if (auto* loop = statement->castTo<CLoop>(); loop != nullptr) {
            statement = statement->getNext();

            loop->getBody()->flatten();
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

void CCompound::enhanceVariables(CGenerator& generator)
{
    auto* firstVariable = child;
    auto* kid = child;

    while (kid != nullptr && kid->getKind() == CNode::kVariable) {
        kid = kid->getNext();
    }

    while (kid != nullptr) {
        auto* assignment = kid->castTo<CBinaryExpression>();

        if (assignment == nullptr || assignment->getOp() != "=") {
            return;
        }

        auto* nameUse = assignment->getLeft()->castTo<CNameUse>();

        if (nameUse == nullptr) {
            return;
        }

        CVariable* variable = nullptr;

        while ((variable = firstVariable->castTo<CVariable>()) != nullptr &&
                variable->getName() != nameUse->getName()) {
            firstVariable = firstVariable->getNext();
        }

        if (variable == nullptr || variable->getInitialValue() != nullptr) {
            return;
        }

        variable->setInitialValue(assignment->getRight());

        kid = kid->getNext();
        delete assignment;

        firstVariable = firstVariable->getNext();
    }
}

void CCompound::enhanceIf(CIf* ifStatement, CGenerator& generator)
{
    if (ifStatement->getThenStatements()->empty() ||
            !ifStatement->getElseStatements()->empty()) {
        return;
    }

    auto* branchStatement = ifStatement->getThenStatements()->getLastChild()->castTo<CBranch>();

    if (branchStatement == nullptr || branchStatement->getLastChild() != nullptr) {
        return;
    }

    auto label = branchStatement->getLabel();
    auto *labelStatement = findLabel(ifStatement, label);

    if (labelStatement == nullptr) {
        return;
    }

    auto* condition = ifStatement->getCondition();

    condition->unlink();
    ifStatement->setCondition(notExpression(condition));

    delete branchStatement;

    for (auto* next = ifStatement->getNext(); ; next = ifStatement->getNext()) {
        ifStatement->addThenStatement(next);

        if (next == labelStatement) {
            break;
        }
    }

    generator.decrementUseCount(label);

    if (ifStatement->getThenStatements()->getLastChild() != nullptr) {
        if (branchStatement = ifStatement->getThenStatements()->getLastChild()->castTo<CBranch>();
                branchStatement != nullptr) {
            label = branchStatement->getLabel();
            labelStatement = findLabel(ifStatement->getParent(), label);

            if (labelStatement == nullptr) {
                return;
            }

            for (auto* next = ifStatement->getParent()->getNext(); ;
                    next = ifStatement->getParent()->getNext()) {
                ifStatement->addElseStatement(next);

                if (next == labelStatement) {
                    break;
                }
            }

            delete branchStatement;
            generator.decrementUseCount(label);
        }
    }
}

void CCompound::enhance(CGenerator& generator)
{
    traverseStatements(this, [this, &generator](CNode* node) {
            if (auto* ifStatement = node->castTo<CIf>(); ifStatement != nullptr) {
                enhanceIf(ifStatement, generator);
            } else if (auto* loop = node->castTo<CLoop>(); loop != nullptr) {
                loop->enhance(generator);
            } else if (auto* assignment = node->castTo<CBinaryExpression>();
                    assignment != nullptr && assignment->getOp() == "=") {
                assignment->enhanceAssignment();
            }
        });

    enhanceVariables(generator);
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

void CIf::addTempDeclaration(CNode* statement)
{
    tempDeclarations->addStatement(statement);
}

void CIf::addThenStatement(CNode* statement)
{
    thenStatements->addStatement(statement);
}

void CIf::addElseStatement(CNode* statement)
{
    elseStatements->addStatement(statement);
}

bool CIf::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherIf = static_cast<CIf*>(other);

    return equalNodes(condition, otherIf->getCondition()) &&
        equalNodes(thenStatements, otherIf->getThenStatements()) &&
        equalNodes(elseStatements, otherIf->getElseStatements());
}

void CIf::generateC(std::ostream& os, CGenerator& generator)
{
    if (condition == nullptr) {
        generator.generateStatement(os, thenStatements);
        return;
    }

    if (resultDeclaration != nullptr) {
        generator.generateStatement(os, resultDeclaration);
        generator.nl(os);
    }

    generator.generateStatement(os, tempDeclarations);

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
    } else if (next != nullptr) {
        generator.nl(os);
    }
}

bool CLoop::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherLoop = static_cast<CLoop*>(other);

    return equalNodes(body, otherLoop->getBody()) &&
        equalNodes(initialize, otherLoop->getInitialize()) &&
        equalNodes(condition, otherLoop->getCondition()) &&
        equalNodes(increment, otherLoop->getIncrement());
}

void CLoop::generateC(std::ostream& os, CGenerator& generator)
{
    switch(loopType) {
        case isDo:
            os << "do {";
            generator.indent();
            body->generateC(os, generator);
            generator.undent();
            generator.nl(os);
            os << "} while (";
            condition->generateC(os, generator);
            os << ");";
            return;

        case isWhile:
            os << "while (";
            condition->generateC(os, generator);
            os << ") {";
            generator.indent();
            body->generateC(os, generator);
            generator.undent();
            generator.nl(os);
            os << '}';
            return;

        case isFor:
            os << "for (";

            if (initialize != nullptr) {
                initialize->generateC(os, generator);
            }

            os << ';';

            if (condition != nullptr) {
                os << ' ';
                condition->generateC(os, generator);
            }

            os << ';';

            if (increment != nullptr) {
                os << ' ';
                increment->generateC(os, generator);
            }

            os << ") {";
            generator.indent();
            body->generateC(os, generator);
            generator.undent();
            generator.nl(os);
            os << '}';
            return;

        default:
            os << "//{loop\n";
            body->generateC(os, generator);
            os << "//loop}\n";
    }
}

void CLoop::enhanceContinues(unsigned loopLabel, CGenerator& generator)
{
    traverseStatements(body, [this, loopLabel, &generator](CNode* node) {
            if (auto* branch = node->castTo<CBranch>();
                    branch != nullptr && branch->getLabel() == loopLabel) {

                for (auto* p = branch->getParent(); p != nullptr; p = p->getParent()) {
                    if (auto* pLoop = p->castTo<CLoop>(); pLoop != nullptr) {
                        if (pLoop == this) {
                            auto* continueStatement = new CContinue;

                            continueStatement->link(branch->getParent(), branch);
                            branch->setNopped(true);
                            generator.decrementUseCount(loopLabel);
                        }

                        break;
                    }
                }
            }
        });

    if (auto* continueStatement = body->getLastChild()->castTo<CContinue>();
            continueStatement != nullptr) {
        delete continueStatement;
    }
}

void CLoop::enhanceBreaks(unsigned loopLabel, CGenerator& generator)
{
    traverseStatements(body, [this, loopLabel, &generator](CNode* node) {
            if (auto* branch = node->castTo<CBranch>();
                    branch != nullptr && branch->getLabel() == loopLabel) {

                for (auto* p = branch->getParent(); p != nullptr; p = p->getParent()) {
                    if (auto* pLoop = p->castTo<CLoop>(); pLoop != nullptr) {
                        if (pLoop == this) {
                            auto* breakStatement = new CBreak;

                            breakStatement->link(branch->getParent(), branch);
                            branch->setNopped(true);
                            generator.decrementUseCount(loopLabel);
                        }

                        break;
                    }
                }
            }
        });
}

void CLoop::tryWhile2For(CGenerator& generator, CIf* ifParent)
{
    CNode* incrementStatement = nullptr;
    CNode* variable = nullptr;

    if (auto* binaryExpression = body->getLastChild()->castTo<CBinaryExpression>();
            binaryExpression != nullptr && binaryPrecedence(binaryExpression->getOp()) == 2) {
        incrementStatement = binaryExpression;
        variable = binaryExpression->getLeft();
    } else if (auto* postfixExpression = body->getLastChild()->castTo<CPostfixExpression>();
            postfixExpression != nullptr) {
        incrementStatement = postfixExpression;
        variable = postfixExpression->getOperand();
    }

    if (incrementStatement != nullptr &&
            (condition->equals(variable) || condition->contains(variable))) {
        loopType = isFor;
        setIncrement(incrementStatement);
    }

    auto* prev = ifParent->getPrevious();

    if (prev == nullptr) {
        return;
    }

    if (auto* binaryExpression = prev->castTo<CBinaryExpression>();
            binaryExpression != nullptr && binaryPrecedence(binaryExpression->getOp()) == 2) {
        variable = binaryExpression->getLeft();

        if (condition->equals(variable) || condition->contains(variable)) {
            loopType = isFor;
            setInitialize(binaryExpression);
        }
    }
}

void CLoop::enhance(CGenerator& generator)
{
    if (loopType != isNone) {
        return;
    }

    body->enhance(generator);

    if (body->getChild() == nullptr) {
        return;
    }

    unsigned loopLabel = ~0u;

    if (auto* label = body->getChild()->castTo<CLabel>(); label != nullptr) {
        loopLabel = label->getLabel();

        if (auto* ifStatement = body->getLastChild()->castTo<CIf>();
                ifStatement != nullptr &&
                ifStatement->getElseStatements()->getChild() == nullptr &&
                ifStatement->getThenStatements()->getChild() != nullptr) {
            if (auto* branch = ifStatement->getThenStatements()->getChild()->castTo<CBranch>();
                    branch != nullptr && branch->getLabel() == loopLabel) {
                loopType = isDo;
                setCondition(ifStatement->getCondition());
                delete ifStatement;
                generator.decrementUseCount(loopLabel);

                if (parent->getParent() != nullptr) {
                    if (auto* ifParent = parent->getParent()->castTo<CIf>();
                            ifParent != nullptr &&
                            ifParent->getElseStatements()->getChild() == nullptr &&
                            ifParent->getThenStatements()->getChild() == this &&
                            ifParent->getThenStatements()->getChild()->getNext() == nullptr &&
                            equalNodes(condition, ifParent->getCondition())) {
                        ifParent->deleteCondition();
                        loopType = isWhile;
                        tryWhile2For(generator, ifParent);
                    }
                }
            }
        }
    }

    if (loopType == isNone) {
        loopType = isFor;

        if (auto* branch = body->getLastChild()->castTo<CBranch>(); branch == nullptr) {
            auto* breakStatement = new CBreak;

            breakStatement->link(body);
        }
    }

    enhanceContinues(loopLabel, generator);

    if (next != nullptr) {
        if (auto* label = next->castTo<CLabel>(); label != nullptr) {
            enhanceBreaks(label->getLabel(), generator);
        }
    }
}

void CLoop::addStatement(CNode* statement)
{
    body->addStatement(statement);
}

bool CSwitch::equals(CNode* other)
{
    if (other->getKind() != kind) {
        return false;
    }

    auto* otherSwitch = static_cast<CSwitch*>(other);
    auto& otherCases = otherSwitch->getCases();

    if (cases.size() != otherCases.size()) {
        return false;
    }

    for (size_t i = 0, c = cases.size(); i < c; ++i) {
        auto* cs = cases[i];
        auto* otherCs = otherCases[i];

        if (cs->value != otherCs->value ||
                !equalNodes(cs->statements, otherCs->statements)) {
            return false;
        }
    }

    return equalNodes(condition, otherSwitch->getCondition()) &&
        equalNodes(defaultStatements, otherSwitch->getDefault());
}

void CSwitch::Case::addStatement(CNode* statement)
{
    statements->addStatement(statement);
}

void CSwitch::addCase(uint64_t value, CNode* statement)
{
    auto* cs = new Case(value);
    cases.push_back(cs);

    if (statement != nullptr) {
        cs->addStatement(statement);
    }
}

void CSwitch::addDefault(CNode* statement)
{
    defaultStatements->addStatement(statement);
}

void CSwitch::generateC(std::ostream& os, CGenerator& generator)
{
    os << "switch (";
    condition->generateC(os, generator);
    os << ") {";

    for (auto* c : cases) {
        generator.nl(os);
        os << "  case " << c->value << ": ";

        generator.indent();
        generator.indent();
        c->statements->generateC(os, generator);
        generator.undent();
        generator.undent();
    }

    if (defaultStatements->getChild() != nullptr) {
        generator.nl(os);
        os << "  default : ";
        generator.indent();
        generator.indent();
        defaultStatements->generateC(os, generator);
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

void CGenerator::tempify()
{
    for (auto& info : expressionStack) {
        if (info.hasSideEffects) {
            auto temp = getTemp(info.type);

            currentCompound->addStatement(new CBinaryExpression("=", new CNameUse(temp), info.expression));
            info.expression = new CNameUse(temp);
            info.hasSideEffects = false;
        }
    }
}

void CGenerator::pushExpression(CNode* expression, ValueType type, bool hasSideEffects)
{
    expressionStack.emplace_back(expression, type, hasSideEffects);
}

CNode* CGenerator::popExpression()
{
    assert(!expressionStack.empty());

    auto* result = expressionStack.back().expression;
    expressionStack.pop_back();

    return result;
}

CNode* CGenerator::getExpression(size_t offset)
{
    assert(offset < expressionStack.size());

    return expressionStack[expressionStack.size() - 1 - offset].expression;
}

void CGenerator::replaceExpression(CNode* expression, size_t offset)
{
    assert(offset < expressionStack.size());

    expressionStack[expressionStack.size() - 1 - offset].expression = expression;
}

std::string CGenerator::getTemp(ValueType type)
{
    auto tempName = "temp_" + toString(temp++);

    tempNode->addStatement(new CVariable(type, tempName));
    return tempName;
}

std::vector<std::string> CGenerator::getTemps(const std::vector<ValueType>& types)
{
    std::vector<std::string> result;
    result.reserve(types.size());

    for (auto& type : types) {
        result.emplace_back(getTemp(type));
    }

    return result;
}

const Local* CGenerator::getLocal(uint32_t localIndex)
{
    auto* function = module->getFunction(codeEntry->getNumber());
    auto* signature = function->getSignature();
    const auto& parameters = signature->getParams();

    if (localIndex < parameters.size()) {
        return parameters[localIndex].get();
    } else {
        return codeEntry->getLocals()[localIndex - parameters.size()].get();
    }
}

std::string CGenerator::localName(Instruction* instruction)
{
    auto localIndex = static_cast<InstructionLocalIdx*>(instruction)->getIndex();

    return getLocal(localIndex)->getCName();
}

unsigned CGenerator::pushLabel(std::vector<ValueType> types)
{
    labelStack.emplace_back(label);
    labelStack.back().types = std::move(types);
    return label++;
}

void CGenerator::popLabel()
{
    assert(!labelStack.empty());

    labelStack.pop_back();
}

std::string CGenerator::globalName(Instruction* instruction)
{
    auto globalIndex = static_cast<InstructionGlobalIdx*>(instruction)->getIndex();
    auto* global = module->getGlobal(globalIndex);

    return global->getCName(module);

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

CCompound* CGenerator::makeBlockResults(const std::vector<ValueType>& types)
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
    auto* previousCompound = currentCompound;
    auto* result = currentCompound =new CCompound();
    auto blockLabel = pushLabel(resultTypes);
    auto label = labelStack.back().label;
    auto stackSize = expressionStack.size();
    auto labelStackSize = labelStack.size();
    auto types = getBlockResults(blockInstruction);
    auto count = types.size();
    auto* resultNode = makeBlockResults(types);
    bool tempifyDone = false;

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
            auto* value = popExpression();

            if (!tempifyDone && value->hasSideEffects()) {
                tempify();
                tempifyDone = true;
            }

            result->addStatement(new CBinaryExpression("=", resultNameNode, value));
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

    currentCompound = previousCompound;
    return result;
}

CNode* CGenerator::generateCLoop(Instruction* instruction)
{
    auto* blockInstruction = static_cast<InstructionBlock*>(instruction);
    auto resultTypes = getBlockResults(blockInstruction);
    auto* previousCompound = currentCompound;
    auto* result = new CLoop;
    currentCompound = result->getBody();
    auto blockLabel = pushLabel(resultTypes);
    auto label = labelStack.back().label;
    auto labelStackSize = labelStack.size();
    auto types = getBlockResults(blockInstruction);
    auto count = types.size();
    auto* resultNode = makeBlockResults(types);
    bool tempifyDone = false;
    std::vector<std::string>& temps = labelStack.back().temps;

    if (resultNode != nullptr) {
        previousCompound->addStatement(resultNode);
    }

    labelStack.back().branchTarget = true;
    labelStack.back().backward = true;

    if (auto* signature = blockInstruction->getSignature(); signature != nullptr) {
        auto& params = signature->getParams();

        if (!params.empty()) {
            temps.reserve(params.size());

            for (auto& param : params) {
                auto tempName = "temp_" + toString(temp++);
                auto* value = popExpression();

                if (!tempifyDone && value->hasSideEffects()) {
                    tempify();
                    tempifyDone = true;
                }

                temps.push_back(tempName);
                result->addStatement(new CVariable(param->getType(), tempName, value));
            }
        }
    }

    auto stackSize = expressionStack.size();

    result->addStatement(new CLabel(blockLabel));

    for (auto i = temps.size(); i-- > 0; ) {
        pushExpression(new CNameUse(temps[i]));
    }

    while (auto* statement = generateCStatement()) {
        result->addStatement(statement);
        if (labelStack.size() < labelStackSize) {
            break;
        }
    }

    for (auto i = count; expressionStack.size() > stackSize && i-- > 0; ) {
        auto* resultNameNode = new CNameUse(makeResultName(label, i));
        auto* value = popExpression();

        if (!tempifyDone && value->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        result->addStatement(new CBinaryExpression("=", resultNameNode, value));
    }

    if (labelStackSize <= labelStack.size()) {
        assert(labelStack.back().label == blockLabel);

        popLabel();
    }

    for (size_t i = 0; i < count; ++i) {
        pushExpression(new CNameUse(makeResultName(label, i)));
    }

    currentCompound = previousCompound;
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
    auto labelStackSize = labelStack.size();
    auto count = types.size();
    auto* resultNode = makeBlockResults(types);
    std::vector<std::string> temps;

    if (resultNode != nullptr) {
        result->setResultDeclaration(resultNode);
    }

    labelStack.back().impliedTarget = true;

    if (auto* signature = blockInstruction->getSignature(); signature != nullptr) {
        auto& params = signature->getParams();

        if (!params.empty()) {
            temps.reserve(params.size());

            for (auto& param : params) {
                auto tempName = "temp_" + toString(temp++);

                temps.push_back(tempName);
                result->addTempDeclaration(new CVariable(param->getType(), tempName, popExpression()));
            }
        }
    }

    auto stackSize = expressionStack.size();

    for (auto i = temps.size(); i-- > 0; ) {
        pushExpression(new CNameUse(temps[i]));
    }

    if (instructionPointer != instructionEnd && instructionPointer->get()->getOpcode() != Opcode::else_) {
        while (auto* statement = generateCStatement()) {
            result->addThenStatement(statement);
            if (labelStack.size() < labelStackSize) {
                break;
            }
        }
    }

    for (auto i = count; expressionStack.size() > stackSize && i-- > 0; ) {
        auto* resultNameNode = new CNameUse(makeResultName(label, i));

        result->addThenStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
    }

    for (auto i = temps.size(); i-- > 0; ) {
        pushExpression(new CNameUse(temps[i]));
    }

    if (instructionPointer != instructionEnd && instructionPointer->get()->getOpcode() == Opcode::else_) {
        ++instructionPointer;

        while (auto* statement = generateCStatement()) {
            result->addElseStatement(statement);
            if (labelStack.size() < labelStackSize) {
                break;
            }
        }
    }

    for (auto i = count; expressionStack.size() > stackSize && i-- > 0; ) {
        auto* resultNameNode = new CNameUse(makeResultName(label, i));

        result->addElseStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
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

CCompound* CGenerator::saveBlockResults(uint32_t index)
{
    auto& labelInfo = getLabel(index);
    auto* compound = new CCompound;
    auto& types = labelInfo.types;

    auto count = types.size();

    for (auto i = count; i-- > 0; ) {
        auto resultName = makeResultName(labelInfo.label, i);
        auto* result = popExpression();
        auto* assignment = new CBinaryExpression("=", new CNameUse(resultName), result);

        compound->addStatement(assignment);
    }

    return compound;
}

void CGenerator::pushBlockResults(uint32_t index)
{
    auto& labelInfo = getLabel(index);
    auto count = labelInfo.types.size();

    for (unsigned i = 0; i < count; ++i) {
        pushExpression(new CNameUse(makeResultName(labelInfo.label, i)));
    }
}

CNode* CGenerator::generateCBranch(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(instruction);
    auto index = branchInstruction->getIndex();
    auto& labelInfo = getLabel(index);

    labelInfo.branchTarget = true;

    if (!labelInfo.backward && !labelInfo.types.empty()) {
        auto* result = saveBlockResults(index);

        result->addStatement(new CBranch(labelInfo.label));
        skipUnreachable(index);

        return result;
    } else if (labelInfo.backward) {
        const auto& temps = labelInfo.temps;

        if (labelInfo.backward && !temps.empty()) {
            auto* result = new CCompound;

            for (const auto& temp : labelInfo.temps) {
                result->addStatement(new CBinaryExpression("=", new CNameUse(temp), popExpression()));
            }

            result->addStatement(new CBranch(labelInfo.label));
            skipUnreachable(index);

            return result;
        }
    }

    skipUnreachable(index);

    return new CBranch(labelInfo.label);
}

CNode* CGenerator::generateCBrIf(CNode* condition, uint32_t index)
{
    auto& labelInfo = getLabel(index);
    auto* ifNode = new CIf(condition);
    auto* branchStatement = new CBranch(labelInfo.label);

    labelInfo.branchTarget = true;

    ifNode->addThenStatement(branchStatement);

    if (!labelInfo.backward && !labelInfo.types.empty()) {
        auto* result = saveBlockResults(index);

        result->addStatement(ifNode);

        pushBlockResults(index);
        return result;
    } else if (labelInfo.backward) {
        const auto& temps = labelInfo.temps;

        if (!temps.empty()) {
            auto* result = new CCompound;

            for (const auto& temp : temps) {
                result->addStatement(new CBinaryExpression("=", new CNameUse(temp), popExpression()));
            }

            result->addStatement(ifNode);

            for (auto i = temps.size(); i-- > 0; ) {
                pushExpression(new CNameUse(temps[i]));
            }

            return result;
        }
    }

    return ifNode;
}

CNode* CGenerator::generateCBrIf(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(instruction);
    auto index = branchInstruction->getIndex();
    auto* condition = popExpression();
    auto* branch = generateCBrIf(condition, index);

    if (condition->hasSideEffects()) {
        tempify();
    }

    return branch;
}

CNode* CGenerator::generateCBrUnless(Instruction* instruction)
{
    auto* branchInstruction = static_cast<InstructionLabelIdx*>(instruction);
    auto index = branchInstruction->getIndex();
    auto* condition = notExpression(popExpression());
    auto* branch = generateCBrIf(condition, index);

    if (condition->hasSideEffects()) {
        tempify();
    }

    return branch;
}

CNode* CGenerator::generateCBrTable(Instruction* instruction)
{
    struct Branch
    {
        Branch(uint32_t label, uint32_t number)
          : label(label), number(number)
        {
        }

        bool operator<(const Branch& other)
        {
            return std::tie(label, number) < std::tie(other.label, other.number);
        }

        uint32_t label;
        uint32_t number;
    };

    auto* branchInstruction = static_cast<InstructionBrTable*>(instruction);
    std::vector<Branch> branches;
    auto defaultLabel = branchInstruction->getDefaultLabel();
    auto& labelInfo = getLabel(defaultLabel);
    const auto& labels = branchInstruction->getLabels();
    bool isComplex = !labelInfo.types.empty() || !labelInfo.temps.empty();
    uint32_t number = 0;

    branches.reserve(labels.size());

    for (auto label : labels) {
        auto& labelInfo = getLabel(label);

        labelInfo.branchTarget = true;
        isComplex = isComplex || !labelInfo.types.empty() || !labelInfo.temps.empty();
        branches.emplace_back(label, number++);
    }

    std::sort(branches.begin(), branches.end());

    auto* index = popExpression();

    if (index->hasSideEffects()) {
        tempify();
    }

    labelInfo.branchTarget = true;

    if (isComplex) {
        auto temp = getTemp(ValueType::i32);
        auto* result = new CCompound;
        result->addStatement(new CBinaryExpression("=", new CNameUse(temp), index));

        for (auto branch : branches) {
            if (branch.label == defaultLabel) {
                continue;
            }

            auto* condition = new CBinaryExpression("==", new CNameUse(temp), new CI32(branch.number));
            result->addStatement(generateCBrIf(condition, branch.label));
        }

        if (!labelInfo.backward && !labelInfo.types.empty()) {
            result->addStatement(saveBlockResults(defaultLabel));
        }

        result->addStatement(new CBranch(labelInfo.label));

        skipUnreachable();

        return result;
    } else {
        auto *result = new CSwitch(index);

        for (auto b = branches.begin(), e = branches.end(); b != e; ++b) {
            const auto& branch = *b;

            if (branch.label == defaultLabel) {
                continue;
            }

            auto n = b + 1;

            if (n != e && n->label == branch.label) {
                result->addCase(branch.number, nullptr);
            } else {
                auto& labelInfo = getLabel(branch.label);

                result->addCase(branch.number, new CBranch(labelInfo.label));
            }
        }

        result->addDefault(new CBranch(labelInfo.label));

        skipUnreachable();

        return result;
    }
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

CNode* CGenerator::generateCShift(std::string_view op, std::string_view type)
{
    auto* right = popExpression();
    auto* left = popExpression();
    std::string_view cast;
    int32_t mod = 0;

    if (type == "i32") {
        mod = 32;
    } else if (type == "u32") {
        cast = "uint32_t";
        mod = 32;
    } else if (type == "i64") {
        mod = 64;
    } else if (type == "u64") {
        cast = "uint64_t";
        mod = 64;
    }

    if (auto value = getIntegerValue(right)) {
        delete right;
        right = new CI32(int32_t(*value) % mod);
    } else {
        right = new CBinaryExpression("%", right, new CI32(mod));
    }

    if (!cast.empty()) {
        left = new CCast(cast, left);
    }

    return new CBinaryExpression(op, left, right);
}

void CGenerator::generateCLoad(std::string_view name, Instruction* instruction, ValueType type)
{
    auto* memory = module->getMemory(0);

    pushExpression(new CLoad(name, memory->getCName(module), makeCombinedOffset(instruction)), type);
}

CNode* CGenerator::generateCLoadSplat(std::string_view splatName, std::string_view loadName,
    Instruction* instruction, ValueType type)
{
    auto* result = new CCall(splatName);

    result->setPure(true);
    generateCLoad(loadName, instruction, type);

    result->addArgument(popExpression());

    return result;
}

CNode* CGenerator::generateCLoadExtend(std::string_view loadName, Instruction* instruction)
{
    auto* result = new CCall(loadName);
    auto* memory = module->getMemory(0);

    result->setPure(true);
    result->addArgument(new CUnaryExpression("&", new CNameUse(memory->getCName(module))));
    result->addArgument(makeCombinedOffset(instruction));

    return result;
}

CNode* CGenerator::generateCStore(std::string_view name, Instruction* instruction)
{
    bool tempifyDone = false;
    auto* valueToStore = popExpression();

    if (valueToStore->hasSideEffects()) {
        tempify();
        tempifyDone = true;
    }

    auto* dynamicOffset = popExpression();

    if (!tempifyDone && dynamicOffset->hasSideEffects()) {
        tempify();
        tempifyDone = true;
    }

    auto* memoryInstruction = static_cast<InstructionMemory*>(instruction);
    auto* memory = module->getMemory(0);
    auto offset = memoryInstruction->getOffset();
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

    auto* store = new CStore(name, memory->getCName(module), combinedOffset, valueToStore);

    return store;
}

void CGenerator::generateCLocalGet(Instruction* instruction)
{
    auto localIndex = static_cast<InstructionLocalIdx*>(instruction)->getIndex();
    auto* local = getLocal(localIndex);

    pushExpression(new CNameUse(local->getCName()), local->getType());
}

CNode* CGenerator::generateCLocalSet(Instruction* instruction)
{
    auto* left = new CNameUse(localName(instruction));
    auto* right = popExpression();
    auto* result = new CBinaryExpression("=", left, right);


    if (right->hasSideEffects()) {
        tempify();
    }

    return result;
}

CNode* CGenerator::generateCLocalTee(Instruction* instruction)
{
    auto* right = popExpression();
    auto* result = new CBinaryExpression("=", new CNameUse(localName(instruction)), right);

    if (right->hasSideEffects()) {
        tempify();
    }

    pushExpression(new CNameUse(localName(instruction)));

    return result;
}

void CGenerator::generateCGlobalGet(Instruction* instruction)
{
    auto globalIndex = static_cast<InstructionGlobalIdx*>(instruction)->getIndex();
    auto* global = module->getGlobal(globalIndex);

    pushExpression(new CNameUse(global->getCName(module)), global->getType(), true);
}

CNode* CGenerator::generateCGlobalSet(Instruction* instruction)
{
    auto* left = new CNameUse(globalName(instruction));
    auto* right = popExpression();
    auto* result = new CBinaryExpression("=", left, right);

    if (right->hasSideEffects()) {
        tempify();
    }

    return result;
}

CNode* CGenerator::generateCCallPredef(std::string_view name, unsigned argumentCount)
{
    auto* call = new CCall(name);
    bool tempifyDone = false;

    call->setPure(true);
    for (unsigned i = 0; i < argumentCount; ++i) {
        auto* argument = popExpression();

        if (!tempifyDone && argument->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        call->addArgument(argument);
    }

    call->reverseArguments();

    return call;
}

CNode* CGenerator::generateCMemorySize()
{
    auto* memory = module->getMemory(0);

    return new CBinaryExpression(".", new CNameUse(memory->getCName(module)),
            new CNameUse("pageCount"));
}

CNode* CGenerator::generateCMemoryCall(std::string_view name, unsigned argumentCount)
{
    auto* memory = module->getMemory(0);
    auto* call = new CCall(name);
    bool tempifyDone = false;

    for (unsigned i = 0; i < argumentCount; ++i) {
        auto* argument = popExpression();

        if (!tempifyDone && argument->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        call->addArgument(argument);
    }

    call->addArgument(new CUnaryExpression("&", new CNameUse(memory->getCName(module))));
    call->reverseArguments();

    return call;
}

CNode* CGenerator::generateCMemoryCopy(Instruction* instruction)
{
    auto* memMemInstruction = static_cast<InstructionMemMem*>(instruction);
    auto* destination = module->getMemory(memMemInstruction->getDestination());
    auto* source = module->getMemory(memMemInstruction->getSource());
    auto* call = new CCall("copyMemory");
    bool tempifyDone = false;

    for (unsigned i = 0; i < 3; ++i) {
        auto* argument = popExpression();

        if (!tempifyDone && argument->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        call->addArgument(argument);
    }

    call->addArgument(new CUnaryExpression("&", new CNameUse(source->getCName(module))));
    call->addArgument(new CUnaryExpression("&", new CNameUse(destination->getCName(module))));
    call->reverseArguments();

    return call;
}

CNode* CGenerator::generateCMemoryInit(Instruction* instruction)
{
    auto* idxMemInstruction = static_cast<InstructionSegmentIdxMem*>(instruction);
    auto* memory = module->getMemory(idxMemInstruction->getMemory());
    auto* segment = module->getSegment(idxMemInstruction->getSegmentIndex());
    auto* call = new CCall("initMemory");
    bool tempifyDone = false;

    for (unsigned i = 0; i < 3; ++i) {
        auto* argument = popExpression();

        if (!tempifyDone && argument->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        call->addArgument(argument);
    }

    call->addArgument(new CNameUse(segment->getCName(module)));
    call->addArgument(new CUnaryExpression("&", new CNameUse(memory->getCName(module))));
    call->reverseArguments();

    return call;
}

CNode* CGenerator::generateCTableCall(Instruction* instruction, std::string_view name,
        unsigned argumentCount)
{
    auto* tableInstruction = static_cast<InstructionTable*>(instruction);
    auto tableIndex = tableInstruction->getIndex();
    auto* table = module->getTable(tableIndex);
    auto* call = new CCall(name);
    bool tempifyDone = false;

    for (unsigned i = 0; i < argumentCount; ++i) {
        auto* argument = popExpression();

        if (!tempifyDone && argument->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        call->addArgument(argument);
    }

    call->addArgument(new CUnaryExpression("&", new CNameUse(table->getCName(module))));
    call->reverseArguments();

    return call;
}

CNode* CGenerator::generateCTableSize(Instruction* instruction)
{
    auto* tableInstruction = static_cast<InstructionTable*>(instruction);
    auto tableIndex = tableInstruction->getIndex();
    auto* table = module->getTable(tableIndex);

    return new CBinaryExpression(".", new CNameUse(table->getCName(module)),
            new CNameUse("elementCount"));
}

CNode* CGenerator::generateCTableInit(Instruction* instruction)
{
    auto* tableElementIdxInstruction = static_cast<InstructionTableElementIdx*>(instruction);
    auto* table = module->getTable(tableElementIdxInstruction->getTableIndex());
    auto* element = module->getElement(tableElementIdxInstruction->getElementIndex());
    auto* call = new CCall("initTable");
    bool tempifyDone = false;

    for (unsigned i = 0; i < 3; ++i) {
        auto* argument = popExpression();

        if (!tempifyDone && argument->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        call->addArgument(argument);
    }

    call->addArgument(new CNameUse(element->getCName(module)));
    call->addArgument(new CUnaryExpression("&", new CNameUse(table->getCName(module))));
    call->reverseArguments();

    return call;
}

CNode* CGenerator::generateCTableCopy(Instruction* instruction)
{
    auto* tableTableInstruction = static_cast<InstructionTableTable*>(instruction);
    auto* destination = module->getTable(tableTableInstruction->getDestination());
    auto* source = module->getTable(tableTableInstruction->getSource());
    auto* call = new CCall("copyTable");
    bool tempifyDone = false;

    for (unsigned i = 0; i < 3; ++i) {
        auto* argument = popExpression();

        if (!tempifyDone && argument->hasSideEffects()) {
            tempify();
            tempifyDone = true;
        }

        call->addArgument(argument);
    }

    call->addArgument(new CUnaryExpression("&", new CNameUse(source->getCName(module))));
    call->addArgument(new CUnaryExpression("&", new CNameUse(destination->getCName(module))));
    call->reverseArguments();

    return call;
}

CNode* CGenerator::generateCTableAccess(Instruction* instruction, bool set)
{
    auto* tableInstruction = static_cast<InstructionTable*>(instruction);
    auto tableIndex = tableInstruction->getIndex();
    auto* table = module->getTable(tableIndex);
    CNode* value = nullptr;

    if (set) {
        value = popExpression();
        if (value->hasSideEffects()) {
            tempify();
        }
    }

    auto* subscript = popExpression();

    if (subscript->hasSideEffects()) {
        tempify();
    }

    auto* data = new CBinaryExpression(".", new CNameUse(table->getCName(module)),
                new CNameUse("data"));
    auto* access = new CSubscript(data, subscript);

    if (set) {
        return new CBinaryExpression("=", access, value);
    } else {
        pushExpression(access);
        return nullptr;
    }
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
    auto hasSideEffects = expressionStack.back().hasSideEffects;
    auto statement = popExpression();

    switch(statement->getKind()) {
        case CNode::kI32:
        case CNode::kI64:
        case CNode::kF32:
        case CNode::kF64:
        case CNode::kV128:
        case CNode::kNameUse:
            {
                delete statement;

                if (hasSideEffects) {
                    tempify();
                }

                return nullptr;
            }

        default:
            return statement;
    }
}

void CGenerator::generateCSelect(Instruction* instruction)
{
    auto* selectInstruction = static_cast<InstructionSelect*>(instruction);
    auto type = selectInstruction->getType();
    bool tempifyDone = false;
    auto* condition = popExpression();

    if (condition->hasSideEffects()) {
        tempify();
        tempifyDone = true;
    }

    auto* falseValue = popExpression();

    if (!tempifyDone && falseValue->hasSideEffects()) {
        tempify();
        tempifyDone = true;
    }

    auto* trueValue = popExpression();

    if (!tempifyDone && trueValue->hasSideEffects()) {
        tempify();
        tempifyDone = true;
    }

    pushExpression(new CTernaryExpression(condition, trueValue, falseValue), type);
}

CNode* CGenerator::generateCReturn(Instruction* instruction)
{
    auto* signature = function->getSignature();
    auto resultTypes = signature->getResults();
    auto count = resultTypes.size();

    skipUnreachable();

    if (count == 0) {
        return new CReturn;
    } else if (count == 1) {
        return new CReturn(popExpression());
    } else {
        auto* result = new CCompound;

        for (auto i = count; i-- > 0; ){
            auto resultName = makeResultName(0, i);
            auto* resultPointerNode = new CNameUse('*' + resultName + "_ptr");

            result->addStatement(new CBinaryExpression("=", resultPointerNode, popExpression()));
        }

        result->addStatement(new CReturn);
        return result;
    }
}

CNode* CGenerator::generateCCall(Instruction* instruction)
{
    auto callInstruction = static_cast<InstructionFunctionIdx*>(instruction);
    auto functionIndex = callInstruction->getIndex();
    auto* calledFunction = module->getFunction(functionIndex);
    auto* signature = calledFunction->getSignature();
    auto* call = new CCall(calledFunction->getCName(module));
    auto& results = signature->getResults();
    std::vector<std::string> temps;
    bool tempifyDone = false;

    for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
        auto hasSideEffects = expressionStack.back().hasSideEffects;
        auto* argument = popExpression();

        call->addArgument(argument);

        if (hasSideEffects && !tempifyDone) {
            tempify();
            tempifyDone = true;
        }
    }

    if (!tempifyDone && results.empty()) {
        tempify();
    }

    if (results.size() > 1) {
        temps = getTemps(results);

        for (auto i = results.size(); i-- > 0; ) {
            call->addArgument(new CUnaryExpression("&", new CNameUse(temps[i])));
        }
    }

    call->reverseArguments();

    if (results.empty()) {
        return call;
    } else if (results.size() == 1) {
        pushExpression(call, results[0]);
        return nullptr;
    } else {
        for (const auto& temp : temps) {
            pushExpression(new CNameUse(temp));
        }

        return call;
    }
}

CNode* CGenerator::generateCCallIndirect(Instruction* instruction)
{
    auto callInstruction = static_cast<InstructionIndirect*>(instruction);
    auto typeIndex = callInstruction->getTypeIndex();
    auto tableIndex = callInstruction->getTableIndex();
    auto hasSideEffects = expressionStack.back().hasSideEffects;
    auto* indexInTable = popExpression();
    auto* call = new CCallIndirect(typeIndex, tableIndex, indexInTable);
    auto* signature = module->getType(typeIndex)->getSignature();
    auto& results = signature->getResults();
    std::vector<std::string> temps;
    bool tempifyDone = false;

    if (hasSideEffects) {
        tempify();
        tempifyDone = true;
    }

    for (size_t i = 0, c = signature->getParams().size(); i < c; ++i) {
        auto hasSideEffects = expressionStack.back().hasSideEffects;
        auto* argument = popExpression();

        call->addArgument(argument);

        if (hasSideEffects && !tempifyDone) {
            tempify();
            tempifyDone = true;
        }
    }

    if (!tempifyDone && results.empty()) {
        tempify();
    }

    if (results.size() > 1) {
        temps = getTemps(results);

        for (auto i = results.size(); i-- > 0; ) {
            call->addArgument(new CUnaryExpression("&", new CNameUse(temps[i])));
        }
    }

    call->reverseArguments();

    if (results.empty()) {
        return call;
    } else if (results.size() == 1) {
        pushExpression(call, results[0]);
        return nullptr;
    } else {
        for (const auto& temp : temps) {
            pushExpression(new CNameUse(temp));
        }

        return call;
    }
}

CNode* CGenerator::generateCExtractLane(Instruction* instruction, const char* type)
{
    auto* laneInstruction = static_cast<InstructionIdx*>(instruction);
    auto index = laneInstruction->getIndex();
    auto* result = new CCall("v128ExtractLane"s + type);

    result->setPure(true);
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

    result->setPure(true);
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

    result->setPure(true);
    auto* newValue = popExpression();

    result->addArgument(popExpression());
    result->addArgument(newValue);

    auto* makeCall = new CCall("v128Makei8x16");

    makeCall->setPure(true);
    for (int i = 0; i < 16; ++i) {
        makeCall->addArgument(new CI32(mask[i]));
    }

    result->addArgument(makeCall);


    return result;
}

CNode* CGenerator::generateCFunctionReference(Instruction* instruction)
{
    auto* indexInstruction = static_cast<InstructionFunctionIdx*>(instruction);
    auto functionIndex = indexInstruction->getIndex();
    auto* function = module->getFunction(functionIndex);
    auto* name = new CNameUse(function->getCName(module));

    return new CCast("void*", name);
}

void CGenerator::generateCFunction()
{
    function = new CFunction(module->getFunction(codeEntry->getNumber())->getSignature());

    for (auto& local : codeEntry->getLocals()) {
        function->addStatement(new CVariable(local->getType(), local->getCName()));
    }

    auto resultTypes = function->getSignature()->getResults();
    auto count = resultTypes.size();

    function->addStatement(tempNode = new CCompound);
    currentCompound = function->getStatements();

    pushLabel(resultTypes);

    auto* resultNode = makeBlockResults(resultTypes);

    if (resultNode != nullptr) {
        function->addStatement(resultNode);
    }

    while (auto* statement = generateCStatement()) {
        function->addStatement(statement);
    }

    bool needsReturn = count != 0 && !expressionStack.empty();

    if (!labelStack.empty() && labelStack.back().branchTarget) {
        if (!expressionStack.empty()) {
            for (auto i = count; i-- > 0; ) {
                auto* resultNameNode = new CNameUse(makeResultName(0, i));

                function->addStatement(new CBinaryExpression("=", resultNameNode, popExpression()));
            }
        }

        function->addStatement(new CLabel(0));

        if (count == 1) {
            function->addStatement(new CReturn(new CNameUse(makeResultName(0))));
        } else {
            for (size_t i = 0; i < count; ++i) {
                auto resultName = makeResultName(0, i);
                auto* resultNameNode  = new CNameUse(resultName);
                auto* resultPointerNode = new CNameUse('*' + resultName + "_ptr");

                function->addStatement(new CBinaryExpression("=", resultPointerNode, resultNameNode));
            }
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
        resultNode = nullptr;
    } else {
        delete resultNode;
        resultNode = nullptr;
    }

    if (enhanced) {
        if (tempNode != nullptr) {
            while (tempNode->getChild() != nullptr) {
                tempNode->getChild()->link(function->getStatements(), tempNode);
            }

            delete tempNode;
        }

        if (resultNode != nullptr) {
            while (resultNode->getChild() != nullptr) {
                resultNode->getChild()->link(function->getStatements(), resultNode);
            }

            delete resultNode;
        }
    }

    popLabel();
    assert(labelStack.empty());
}

CNode* CGenerator::generateCStatement()
{
    while (instructionPointer != instructionEnd) {
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
                statement = generateCBranch(instruction);
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
                statement = generateCCall(instruction);
                break;

            case Opcode::call_indirect:
                statement = generateCCallIndirect(instruction);
                break;

    //      case Opcode::return_call:
    //      case Opcode::return_call_indirect:

            case Opcode::drop:
                statement = generateCDrop(instruction);
                break;

            case Opcode::select:
                generateCSelect(instruction);
                break;

            case Opcode::local__get:
                generateCLocalGet(instruction);
                break;

            case Opcode::local__set:
                statement = generateCLocalSet(instruction);
                break;

            case Opcode::local__tee:
                statement = generateCLocalTee(instruction);
                break;

            case Opcode::global__get:
                generateCGlobalGet(instruction);
                break;

            case Opcode::global__set:
                statement = generateCGlobalSet(instruction);
                break;

            case Opcode::table__get:
                generateCTableAccess(instruction, false);
                break;

            case Opcode::table__set:
                statement = generateCTableAccess(instruction, true);
                break;

            case Opcode::i32__load:
                generateCLoad("loadI32", instruction, ValueType::i32);
                break;

            case Opcode::i64__load:
                generateCLoad("loadI64", instruction, ValueType::i64);
                break;

            case Opcode::f32__load:
                generateCLoad("loadF32", instruction, ValueType::f32);
                break;

            case Opcode::f64__load:
                generateCLoad("loadF64", instruction, ValueType::f64);
                break;

            case Opcode::i32__load8_s:
                generateCLoad("loadI32I8", instruction, ValueType::i32);
                break;

            case Opcode::i32__load8_u:
                generateCLoad("loadI32U8", instruction, ValueType::i32);
                break;

            case Opcode::i32__load16_s:
                generateCLoad("loadI32I16", instruction, ValueType::i32);
                break;

            case Opcode::i32__load16_u:
                generateCLoad("loadI32U16", instruction, ValueType::i32);
                break;

            case Opcode::i64__load8_s:
                generateCLoad("loadI64I8", instruction, ValueType::i64);
                break;

            case Opcode::i64__load8_u:
                generateCLoad("loadI64U8", instruction, ValueType::i64);
                break;

            case Opcode::i64__load16_s:
                generateCLoad("loadI64I16", instruction, ValueType::i64);
                break;

            case Opcode::i64__load16_u:
                generateCLoad("loadI64U16", instruction, ValueType::i64);
                break;

            case Opcode::i64__load32_s:
                generateCLoad("loadI64I32", instruction, ValueType::i64);
                break;

            case Opcode::i64__load32_u:
                generateCLoad("loadI64U32", instruction, ValueType::i64);
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
                pushExpression(new CI32(static_cast<InstructionI32*>(instruction)->getValue()), ValueType::i32);
                break;

            case Opcode::i64__const:
                pushExpression(new CI64(static_cast<InstructionI64*>(instruction)->getValue()), ValueType::i64);
                break;

            case Opcode::f32__const:
                pushExpression(new CF32(static_cast<InstructionF32*>(instruction)->getValue()), ValueType::f32);
                break;

            case Opcode::f64__const:
                pushExpression(new CF64(static_cast<InstructionF64*>(instruction)->getValue()), ValueType::f64);
                break;

            case Opcode::i32__eqz:
            case Opcode::i64__eqz:
                pushExpression(generateCUnaryExpression("!"), ValueType::i32);
                break;

            case Opcode::i32__eq:
            case Opcode::i64__eq:
            case Opcode::f32__eq:
            case Opcode::f64__eq:
                pushExpression(generateCBinaryExpression("=="), ValueType::i32);
                break;

            case Opcode::i32__ne:
            case Opcode::i64__ne:
            case Opcode::f32__ne:
            case Opcode::f64__ne:
                pushExpression(generateCBinaryExpression("!="), ValueType::i32);
                break;

            case Opcode::i32__lt_s:
            case Opcode::i64__lt_s:
            case Opcode::f32__lt:
            case Opcode::f64__lt:
                pushExpression(generateCBinaryExpression("<"), ValueType::i32);
                break;

            case Opcode::i32__lt_u:
                pushExpression(generateCUBinaryExpression("<", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i64__lt_u:
                pushExpression(generateCUBinaryExpression("<", "uint64_t"), ValueType::i32);
                break;

            case Opcode::i32__gt_s:
            case Opcode::i64__gt_s:
            case Opcode::f32__gt:
            case Opcode::f64__gt:
                pushExpression(generateCBinaryExpression(">"), ValueType::i32);
                break;

            case Opcode::i32__gt_u:
                pushExpression(generateCUBinaryExpression(">", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i64__gt_u:
                pushExpression(generateCUBinaryExpression(">", "uint64_t"), ValueType::i32);
                break;

            case Opcode::i32__le_s:
            case Opcode::i64__le_s:
            case Opcode::f32__le:
            case Opcode::f64__le:
                pushExpression(generateCBinaryExpression("<="), ValueType::i32);
                break;

            case Opcode::i32__le_u:
                pushExpression(generateCUBinaryExpression("<=", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i64__le_u:
                pushExpression(generateCUBinaryExpression("<=", "uint64_t"), ValueType::i32);
                break;

            case Opcode::i32__ge_s:
            case Opcode::i64__ge_s:
            case Opcode::f32__ge:
            case Opcode::f64__ge:
                pushExpression(generateCBinaryExpression(">="), ValueType::i32);
                break;

            case Opcode::i32__ge_u:
                pushExpression(generateCUBinaryExpression(">=", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i64__ge_u:
                pushExpression(generateCUBinaryExpression(">=", "uint64_t"), ValueType::i32);
                break;

            case Opcode::i32__clz:
                pushExpression(generateCCallPredef("clz32", 1), ValueType::i32);
                break;

            case Opcode::i32__ctz:
                pushExpression(generateCCallPredef("ctz32", 1), ValueType::i32);
                break;

            case Opcode::i32__popcnt:
                pushExpression(generateCCallPredef("popcnt32", 1), ValueType::i32);
                break;

            case Opcode::i32__add:
                pushExpression(generateCBinaryExpression("+"), ValueType::i32);
                break;

            case Opcode::i64__add:
                pushExpression(generateCBinaryExpression("+"), ValueType::i64);
                break;

            case Opcode::f32__add:
                pushExpression(generateCBinaryExpression("+"), ValueType::f32);
                break;

            case Opcode::f64__add:
                pushExpression(generateCBinaryExpression("+"), ValueType::f64);
                break;

            case Opcode::i32__sub:
                pushExpression(generateCBinaryExpression("-"), ValueType::i32);
                break;

            case Opcode::i64__sub:
                pushExpression(generateCBinaryExpression("-"), ValueType::i64);
                break;

            case Opcode::f32__sub:
                pushExpression(generateCBinaryExpression("-"), ValueType::f32);
                break;

            case Opcode::f64__sub:
                pushExpression(generateCBinaryExpression("-"), ValueType::f64);
                break;

            case Opcode::i32__mul:
                pushExpression(generateCBinaryExpression("*"), ValueType::i32);
                break;

            case Opcode::i64__mul:
                pushExpression(generateCBinaryExpression("*"), ValueType::i64);
                break;

            case Opcode::f32__mul:
                pushExpression(generateCBinaryExpression("*"), ValueType::f32);
                break;

            case Opcode::f64__mul:
                pushExpression(generateCBinaryExpression("*"), ValueType::f64);
                break;

            case Opcode::i32__div_s:
                pushExpression(generateCBinaryExpression("/"), ValueType::i32);
                break;

            case Opcode::i64__div_s:
                pushExpression(generateCBinaryExpression("/"), ValueType::i64);
                break;

            case Opcode::f32__div:
                pushExpression(generateCBinaryExpression("/"), ValueType::f32);
                break;

            case Opcode::f64__div:
                pushExpression(generateCBinaryExpression("/"), ValueType::f64);
                break;

            case Opcode::i32__div_u:
                pushExpression(generateCUBinaryExpression("/", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i64__div_u:
                pushExpression(generateCUBinaryExpression("/", "uint64_t"), ValueType::i64);
                break;

            case Opcode::i32__rem_s:
                pushExpression(generateCBinaryExpression("%"), ValueType::i32);
                break;

            case Opcode::i64__rem_s:
                pushExpression(generateCBinaryExpression("%"), ValueType::i64);
                break;

            case Opcode::i32__rem_u:
                pushExpression(generateCUBinaryExpression("%", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i64__rem_u:
                pushExpression(generateCUBinaryExpression("%", "uint64_t"), ValueType::i64);
                break;

            case Opcode::i32__and:
                pushExpression(generateCBinaryExpression("&"), ValueType::i32);
                break;

            case Opcode::i64__and:
                pushExpression(generateCBinaryExpression("&"), ValueType::i64);
                break;

            case Opcode::i32__or:
                pushExpression(generateCBinaryExpression("|"), ValueType::i32);
                break;

            case Opcode::i64__or:
                pushExpression(generateCBinaryExpression("|"), ValueType::i64);
                break;

            case Opcode::i32__xor:
                pushExpression(generateCBinaryExpression("^"), ValueType::i32);
                break;

            case Opcode::i64__xor:
                pushExpression(generateCBinaryExpression("^"), ValueType::i64);
                break;

            case Opcode::i32__shl:
                pushExpression(generateCShift("<<", "i32"), ValueType::i32);
                break;

            case Opcode::i64__shl:
                pushExpression(generateCShift("<<", "i64"), ValueType::i64);
                break;

            case Opcode::i32__shr_s:
                pushExpression(generateCShift(">>", "i32"), ValueType::i32);
                break;

            case Opcode::i64__shr_s:
                pushExpression(generateCShift(">>", "i64"), ValueType::i64);
                break;

            case Opcode::i32__shr_u:
                pushExpression(generateCShift(">>", "u32"), ValueType::i32);
                break;

            case Opcode::i64__shr_u:
                pushExpression(generateCShift(">>", "u64"), ValueType::i64);
                break;

            case Opcode::i32__rotl:
                pushExpression(generateCCallPredef("rotl32", 2), ValueType::i32);
                break;

            case Opcode::i32__rotr:
                pushExpression(generateCCallPredef("rotr32", 2), ValueType::i32);
                break;

            case Opcode::i64__clz:
                pushExpression(generateCCallPredef("clz64", 1), ValueType::i64);
                break;

            case Opcode::i64__ctz:
                pushExpression(generateCCallPredef("ctz64", 1), ValueType::i64);
                break;

            case Opcode::i64__popcnt:
                pushExpression(generateCCallPredef("popcnt64", 1), ValueType::i64);
                break;

            case Opcode::i64__rotl:
                pushExpression(generateCCallPredef("rotl64", 2), ValueType::i64);
                break;

            case Opcode::i64__rotr:
                pushExpression(generateCCallPredef("rotr64", 2), ValueType::i64);
                break;

            case Opcode::f32__neg:
                pushExpression(generateCUnaryExpression("-"), ValueType::f32);
                break;

            case Opcode::f64__neg:
                pushExpression(generateCUnaryExpression("-"), ValueType::f64);
                break;

            case Opcode::f32__abs:
                pushExpression(generateCCallPredef("fabsf", 1), ValueType::f32);
                break;

            case Opcode::f64__abs:
                pushExpression(generateCCallPredef("fabs", 1), ValueType::f64);
                break;

            case Opcode::f32__ceil:
                pushExpression(generateCCallPredef("ceilf", 1), ValueType::f32);
                break;

            case Opcode::f64__ceil:
                pushExpression(generateCCallPredef("ceil", 1), ValueType::f64);
                break;

            case Opcode::f32__floor:
                pushExpression(generateCCallPredef("floorf", 1), ValueType::f32);
                break;

            case Opcode::f64__floor:
                pushExpression(generateCCallPredef("floor", 1), ValueType::f64);
                break;

            case Opcode::f32__trunc:
                pushExpression(generateCCallPredef("truncf", 1), ValueType::f32);
                break;

            case Opcode::f64__trunc:
                pushExpression(generateCCallPredef("trunc", 1), ValueType::f64);
                break;

            case Opcode::f32__nearest:
                pushExpression(generateCCallPredef("nearbyintf", 1), ValueType::f32);
                break;

            case Opcode::f64__nearest:
                pushExpression(generateCCallPredef("nearbyint", 1), ValueType::f64);
                break;

            case Opcode::f32__sqrt:
                pushExpression(generateCCallPredef("sqrtf", 1), ValueType::f32);
                break;

            case Opcode::f64__sqrt:
                pushExpression(generateCCallPredef("sqrt", 1), ValueType::f64);
                break;

            case Opcode::f32__min:
                pushExpression(generateCCallPredef("minF32", 2), ValueType::f32);
                break;

            case Opcode::f64__min:
                pushExpression(generateCCallPredef("minF64", 2), ValueType::f64);
                break;

            case Opcode::f32__max:
                pushExpression(generateCCallPredef("maxF32", 2), ValueType::f32);
                break;

            case Opcode::f64__max:
                pushExpression(generateCCallPredef("maxF64", 2), ValueType::f64);
                break;

            case Opcode::f32__copysign:
                pushExpression(generateCCallPredef("copysignf", 2), ValueType::f32);
                break;

            case Opcode::f64__copysign:
                pushExpression(generateCCallPredef("copysign", 2), ValueType::f64);
                break;


            case Opcode::i32__trunc_f32_s:
                pushExpression(generateCCast("int32_t"), ValueType::i32);
                break;

            case Opcode::i32__trunc_f64_s:
                pushExpression(generateCCast("int32_t"), ValueType::i32);
                break;

            case Opcode::i32__wrap_i64:
                pushExpression(generateCCast("int32_t"), ValueType::i32);
                break;

            case Opcode::i32__trunc_f32_u:
                pushExpression(generateCDoubleCast("int32_t", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i32__trunc_f64_u:
                pushExpression(generateCDoubleCast("int32_t", "uint32_t"), ValueType::i32);
                break;

            case Opcode::i64__trunc_f32_s:
                pushExpression(generateCCast("int64_t"), ValueType::i64);
                break;

            case Opcode::i64__trunc_f64_s:
                pushExpression(generateCCast("int64_t"), ValueType::i64);
                break;

            case Opcode::i64__extend_i32_s:
                pushExpression(generateCCast("int64_t"), ValueType::i64);
                break;

            case Opcode::i64__trunc_f32_u:
                pushExpression(generateCDoubleCast("int64_t", "uint64_t"), ValueType::i64);
                break;

            case Opcode::i64__trunc_f64_u:
                pushExpression(generateCDoubleCast("int64_t", "uint64_t"), ValueType::i64);
                break;

            case Opcode::i64__extend_i32_u:
                pushExpression(generateCDoubleCast("int64_t", "uint32_t"), ValueType::i64);
                break;

            case Opcode::f32__convert_i32_s:
                pushExpression(generateCCast("float"), ValueType::f32);
                break;

            case Opcode::f32__convert_i64_s:
                pushExpression(generateCCast("float"), ValueType::f32);
                break;

            case Opcode::f32__demote_f64:
                pushExpression(generateCCast("float"), ValueType::f32);
                break;

            case Opcode::f32__convert_i32_u:
                pushExpression(generateCDoubleCast("float", "uint32_t"), ValueType::f32);
                break;

            case Opcode::f32__convert_i64_u:
                pushExpression(generateCDoubleCast("float", "uint64_t"), ValueType::f32);
                break;

            case Opcode::f64__convert_i32_s:
                pushExpression(generateCCast("double"), ValueType::f64);
                break;

            case Opcode::f64__convert_i64_s:
                pushExpression(generateCCast("double"), ValueType::f64);
                break;

            case Opcode::f64__promote_f32:
                pushExpression(generateCCast("double"), ValueType::f64);
                break;

            case Opcode::f64__convert_i32_u:
                pushExpression(generateCDoubleCast("double", "uint32_t"), ValueType::f64);
                break;

            case Opcode::f64__convert_i64_u:
                pushExpression(generateCDoubleCast("double", "uint64_t"), ValueType::f64);
                break;

            case Opcode::i32__reinterpret_f32:
                pushExpression(generateCCallPredef("reinterpretI32F32", 1), ValueType::i32);
                break;

            case Opcode::i64__reinterpret_f64:
                pushExpression(generateCCallPredef("reinterpretI64F64", 1), ValueType::i64);
                break;

            case Opcode::f32__reinterpret_i32:
                pushExpression(generateCCallPredef("reinterpretF32I32", 1), ValueType::f32);
                break;

            case Opcode::f64__reinterpret_i64:
                pushExpression(generateCCallPredef("reinterpretF64I64", 1), ValueType::f64);
                break;

            case Opcode::memory__size:
                pushExpression(generateCMemorySize(), ValueType::i32);
                break;

            case Opcode::memory__grow:
                pushExpression(generateCMemoryCall("growMemory", 1), ValueType::i32);
                break;

            case Opcode::i32__extend8_s:
                pushExpression(generateCDoubleCast("int32_t", "int8_t"), ValueType::i32);
                break;

            case Opcode::i32__extend16_s:
                pushExpression(generateCDoubleCast("int32_t", "int16_t"), ValueType::i32);
                break;

            case Opcode::i64__extend8_s:
                pushExpression(generateCDoubleCast("int64_t", "int8_t"), ValueType::i64);
                break;

            case Opcode::i64__extend16_s:
                pushExpression(generateCDoubleCast("int64_t", "int16_t"), ValueType::i64);
                break;

            case Opcode::i64__extend32_s:
                pushExpression(generateCDoubleCast("int64_t", "int32_t"), ValueType::i64);
                break;

            case Opcode::ref__null:
                pushExpression(new CNameUse("NULL"));
                break;

            case Opcode::ref__is_null:
                pushExpression(generateCUnaryExpression("!"), ValueType::i32);
                break;

            case Opcode::ref__func:
                pushExpression(generateCFunctionReference(instruction));
                break;

    //      case Opcode::alloca:

            case Opcode::br_unless:
                statement = generateCBrUnless(instruction);
                break;

    //      case Opcode::call_host:
    //      case Opcode::data:
    //      case Opcode::drop_keep:

            //EXTNS
            case Opcode::i32__trunc_sat_f32_s:
                pushExpression(generateCCallPredef("satI32F32", 1), ValueType::i32);
                break;

            case Opcode::i32__trunc_sat_f32_u:
                pushExpression(generateCCallPredef("satU32F32", 1), ValueType::i32);
                break;

            case Opcode::i32__trunc_sat_f64_s:
                pushExpression(generateCCallPredef("satI32F64", 1), ValueType::i32);
                break;

            case Opcode::i32__trunc_sat_f64_u:
                pushExpression(generateCCallPredef("satU32F64", 1), ValueType::i32);
                break;

            case Opcode::i64__trunc_sat_f32_s:
                pushExpression(generateCCallPredef("satI64F32", 1), ValueType::i64);
                break;

            case Opcode::i64__trunc_sat_f32_u:
                pushExpression(generateCCallPredef("satU64F32", 1), ValueType::i64);
                break;

            case Opcode::i64__trunc_sat_f64_s:
                pushExpression(generateCCallPredef("satI64F64", 1), ValueType::i64);
                break;

            case Opcode::i64__trunc_sat_f64_u:
                pushExpression(generateCCallPredef("satU64F64", 1), ValueType::i64);
                break;

            case Opcode::memory__init:
                statement = generateCMemoryInit(instruction);
                break;

            case Opcode::data__drop:
                // nop
                break;

            case Opcode::memory__copy:
                statement = generateCMemoryCopy(instruction);
                break;

            case Opcode::memory__fill:
                statement = generateCMemoryCall("fillMemory", 3);
                break;

            case Opcode::table__init:
                statement = generateCTableInit(instruction);
                break;

            case Opcode::elem__drop:
                // nop
                break;

            case Opcode::table__copy:
                statement = generateCTableCopy(instruction);
                break;

            case Opcode::table__grow:
                pushExpression(generateCTableCall(instruction, "growTable", 1), ValueType::i32);
                break;

            case Opcode::table__size:
                pushExpression(generateCTableSize(instruction), ValueType::i32);
                break;

            case Opcode::table__fill:
                statement = generateCTableCall(instruction, "fillTable", 3);
                break;


            // SIMD
            case Opcode::v128__load:
                generateCLoad("loadV128", instruction, ValueType::v128);
                break;

            case Opcode::i16x8__load8x8_s:
                pushExpression(generateCLoadExtend("v128SLoadExti16x8", instruction), ValueType::v128);
                break;

            case Opcode::i16x8__load8x8_u:
                pushExpression(generateCLoadExtend("v128SLoadExtu16x8", instruction), ValueType::v128);
                break;

            case Opcode::i32x4__load16x4_s:
                pushExpression(generateCLoadExtend("v128SLoadExti32x4", instruction), ValueType::v128);
                break;

            case Opcode::i32x4__load16x4_u:
                pushExpression(generateCLoadExtend("v128SLoadExtu32x4", instruction), ValueType::v128);
                break;

            case Opcode::i64x2__load32x2_s:
                pushExpression(generateCLoadExtend("v128SLoadExti64x2", instruction), ValueType::v128);
                break;

            case Opcode::i64x2__load32x2_u:
                pushExpression(generateCLoadExtend("v128SLoadExtu64x2", instruction), ValueType::v128);
                break;

            case Opcode::v8x16__load_splat:
                pushExpression(generateCLoadSplat("v128Splati8x16", "loadI32", instruction, ValueType::i32), ValueType::v128);
                break;

            case Opcode::v16x8__load_splat:
                pushExpression(generateCLoadSplat("v128Splati16x8", "loadI32", instruction, ValueType::i32), ValueType::v128);
                break;

            case Opcode::v32x4__load_splat:
                pushExpression(generateCLoadSplat("v128Splati32x4", "loadI32", instruction, ValueType::i32), ValueType::v128);
                break;

            case Opcode::v64x2__load_splat:
                pushExpression(generateCLoadSplat("v128Splati64x2", "loadI64", instruction, ValueType::i64), ValueType::v128);
                break;

            case Opcode::v128__store:
                statement = generateCStore("storeV128", instruction);
                break;

            case Opcode::v128__const:
                pushExpression(new CV128(static_cast<InstructionV128*>(instruction)->getValue()), ValueType::v128);
                break;

            case Opcode::v8x16__shuffle:
                pushExpression(generateCShuffle(instruction), ValueType::v128);
                break;

            case Opcode::v8x16__swizzle:
                pushExpression(generateCCallPredef("v128Swizzlei8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__splat:
                pushExpression(generateCCallPredef("v128Splati8x16", 1), ValueType::v128);
                break;

            case Opcode::i16x8__splat:
                pushExpression(generateCCallPredef("v128Splati16x8", 1), ValueType::v128);
                break;

            case Opcode::i32x4__splat:
                pushExpression(generateCCallPredef("v128Splati32x4", 1), ValueType::v128);
                break;

            case Opcode::i64x2__splat:
                pushExpression(generateCCallPredef("v128Splati64x2", 1), ValueType::v128);
                break;

            case Opcode::f32x4__splat:
                pushExpression(generateCCallPredef("v128Splatf32x4", 1), ValueType::v128);
                break;

            case Opcode::f64x2__splat:
                pushExpression(generateCCallPredef("v128Splatf64x2", 1), ValueType::v128);
                break;

            case Opcode::i8x16__extract_lane_s:
                pushExpression(generateCExtractLane(instruction, "i8x16"), ValueType::v128);
                break;

            case Opcode::i8x16__extract_lane_u:
                pushExpression(generateCExtractLane(instruction, "u8x16"), ValueType::v128);
                break;

            case Opcode::i8x16__replace_lane:
                pushExpression(generateCReplaceLane(instruction, "i8x16"), ValueType::v128);
                break;

            case Opcode::i16x8__extract_lane_s:
                pushExpression(generateCExtractLane(instruction, "i16x8"), ValueType::v128);
                break;

            case Opcode::i16x8__extract_lane_u:
                pushExpression(generateCExtractLane(instruction, "u16x8"), ValueType::v128);
                break;

            case Opcode::i16x8__replace_lane:
                pushExpression(generateCReplaceLane(instruction, "i16x8"), ValueType::v128);
                break;

            case Opcode::i32x4__extract_lane:
                pushExpression(generateCExtractLane(instruction, "i32x4"), ValueType::v128);
                break;

            case Opcode::i32x4__replace_lane:
                pushExpression(generateCReplaceLane(instruction, "i32x4"), ValueType::v128);
                break;

            case Opcode::i64x2__extract_lane:
                pushExpression(generateCExtractLane(instruction, "i64x2"), ValueType::v128);
                break;

            case Opcode::i64x2__replace_lane:
                pushExpression(generateCReplaceLane(instruction, "i64x2"), ValueType::v128);
                break;

            case Opcode::f32x4__extract_lane:
                pushExpression(generateCExtractLane(instruction, "f32x4"), ValueType::v128);
                break;

            case Opcode::f32x4__replace_lane:
                pushExpression(generateCReplaceLane(instruction, "f32x4"), ValueType::v128);
                break;

            case Opcode::f64x2__extract_lane:
                pushExpression(generateCExtractLane(instruction, "f64x2"), ValueType::v128);
                break;

            case Opcode::f64x2__replace_lane:
                pushExpression(generateCReplaceLane(instruction, "f64x2"), ValueType::v128);
                break;

            case Opcode::i8x16__eq:
                pushExpression(generateCCallPredef("v128Eqi8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__ne:
                pushExpression(generateCCallPredef("v128Nei8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__lt_s:
                pushExpression(generateCCallPredef("v128Lti8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__lt_u:
                pushExpression(generateCCallPredef("v128Ltu8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__gt_s:
                pushExpression(generateCCallPredef("v128Gti8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__gt_u:
                pushExpression(generateCCallPredef("v128Gtu8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__le_s:
                pushExpression(generateCCallPredef("v128Lei8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__le_u:
                pushExpression(generateCCallPredef("v128Leu8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__ge_s:
                pushExpression(generateCCallPredef("v128Gei8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__ge_u:
                pushExpression(generateCCallPredef("v128Geu8x16", 2), ValueType::v128);
                break;

            case Opcode::i16x8__eq:
                pushExpression(generateCCallPredef("v128Eqi16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__ne:
                pushExpression(generateCCallPredef("v128Nei16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__lt_s:
                pushExpression(generateCCallPredef("v128Lti16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__lt_u:
                pushExpression(generateCCallPredef("v128Ltu16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__gt_s:
                pushExpression(generateCCallPredef("v128Gti16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__gt_u:
                pushExpression(generateCCallPredef("v128Gtu16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__le_s:
                pushExpression(generateCCallPredef("v128Lei16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__le_u:
                pushExpression(generateCCallPredef("v128Leu16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__ge_s:
                pushExpression(generateCCallPredef("v128Gei16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__ge_u:
                pushExpression(generateCCallPredef("v128Geu16x8", 2), ValueType::v128);
                break;

            case Opcode::i32x4__eq:
                pushExpression(generateCCallPredef("v128Eqi32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__ne:
                pushExpression(generateCCallPredef("v128Nei32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__lt_s:
                pushExpression(generateCCallPredef("v128Lti32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__lt_u:
                pushExpression(generateCCallPredef("v128Ltu32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__gt_s:
                pushExpression(generateCCallPredef("v128Gti32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__gt_u:
                pushExpression(generateCCallPredef("v128Gtu32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__le_s:
                pushExpression(generateCCallPredef("v128Lei32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__le_u:
                pushExpression(generateCCallPredef("v128Leu32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__ge_s:
                pushExpression(generateCCallPredef("v128Gei32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__ge_u:
                pushExpression(generateCCallPredef("v128Geu32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__eq:
                pushExpression(generateCCallPredef("v128Eqf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__ne:
                pushExpression(generateCCallPredef("v128Nef32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__lt:
                pushExpression(generateCCallPredef("v128Ltf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__gt:
                pushExpression(generateCCallPredef("v128Gtf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__le:
                pushExpression(generateCCallPredef("v128Lef32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__ge:
                pushExpression(generateCCallPredef("v128Gef32x4", 2), ValueType::v128);
                break;

            case Opcode::f64x2__eq:
                pushExpression(generateCCallPredef("v128Eqf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__ne:
                pushExpression(generateCCallPredef("v128Nef64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__lt:
                pushExpression(generateCCallPredef("v128Ltf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__gt:
                pushExpression(generateCCallPredef("v128Gtf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__le:
                pushExpression(generateCCallPredef("v128Lef64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__ge:
                pushExpression(generateCCallPredef("v128Gef64x2", 2), ValueType::v128);
                break;

            case Opcode::v128__not:
                pushExpression(generateCCallPredef("v128Noti64x2", 1), ValueType::v128);
                break;

            case Opcode::v128__and:
                pushExpression(generateCCallPredef("v128Andi64x2", 2), ValueType::v128);
                break;

            case Opcode::v128__andnot:
                pushExpression(generateCCallPredef("v128AndNoti64x2", 2), ValueType::v128);
                break;

            case Opcode::v128__or:
                pushExpression(generateCCallPredef("v128Ori64x2", 2), ValueType::v128);
                break;

            case Opcode::v128__xor:
                pushExpression(generateCCallPredef("v128Xori64x2", 2), ValueType::v128);
                break;

            case Opcode::v128__bitselect:
                pushExpression(generateCCallPredef("v128Bitselect", 3), ValueType::v128);
                break;

            case Opcode::i8x16__abs:
                pushExpression(generateCCallPredef("v128Absi8x16", 1), ValueType::v128);
                break;

            case Opcode::i8x16__neg:
                pushExpression(generateCCallPredef("v128Negi8x16", 1), ValueType::v128);
                break;

            case Opcode::i8x16__any_true:
                pushExpression(generateCCallPredef("v128SAnyTruei8x16", 1), ValueType::v128);
                break;

            case Opcode::i8x16__all_true:
                pushExpression(generateCCallPredef("v128SAllTruei8x16", 1), ValueType::v128);
                break;

            case Opcode::i8x16__narrow_i16x8_s:
                pushExpression(generateCCallPredef("narrowI8x16I16x8", 2), ValueType::v128);
                break;

            case Opcode::i8x16__narrow_i16x8_u:
                pushExpression(generateCCallPredef("narrowU8x16I16x8", 2), ValueType::v128);
                break;

            case Opcode::i8x16__shl:
                pushExpression(generateCCallPredef("v128Shli8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__shr_s:
                pushExpression(generateCCallPredef("v128Shri8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__shr_u:
                pushExpression(generateCCallPredef("v128Shru8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__add:
                pushExpression(generateCCallPredef("v128Addi8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__add_saturate_s:
                pushExpression(generateCCallPredef("v128SatAddi8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__add_saturate_u:
                pushExpression(generateCCallPredef("v128SatAddu8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__sub:
                pushExpression(generateCCallPredef("v128Subi8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__sub_saturate_s:
                pushExpression(generateCCallPredef("v128SatSubi8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__sub_saturate_u:
                pushExpression(generateCCallPredef("v128SatSubu8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__min_s:
                pushExpression(generateCCallPredef("v128Mini8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__min_u:
                pushExpression(generateCCallPredef("v128Minu8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__max_s:
                pushExpression(generateCCallPredef("v128Maxi8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__max_u:
                pushExpression(generateCCallPredef("v128Maxu8x16", 2), ValueType::v128);
                break;

            case Opcode::i8x16__avgr_u:
                pushExpression(generateCCallPredef("v128Avgru8x16", 2), ValueType::v128);
                break;

            case Opcode::i16x8__abs:
                pushExpression(generateCCallPredef("v128Absi16x8", 1), ValueType::v128);
                break;

            case Opcode::i16x8__neg:
                pushExpression(generateCCallPredef("v128Negi16x8", 1), ValueType::v128);
                break;

            case Opcode::i16x8__any_true:
                pushExpression(generateCCallPredef("v128SAnyTruei16x8", 1), ValueType::v128);
                break;

            case Opcode::i16x8__all_true:
                pushExpression(generateCCallPredef("v128SAllTruei16x8", 1), ValueType::v128);
                break;

            case Opcode::i16x8__narrow_i32x4_s:
                pushExpression(generateCCallPredef("narrowI16x8I32x4", 2), ValueType::v128);
                break;

            case Opcode::i16x8__narrow_i32x4_u:
                pushExpression(generateCCallPredef("narrowU16x8I32x4", 2), ValueType::v128);
                break;

            case Opcode::i16x8__widen_low_i8x16_s:
                pushExpression(generateCCallPredef("v128WidenLowi16x8i8x16", 1), ValueType::v128);
                break;

            case Opcode::i16x8__widen_high_i8x16_s:
                pushExpression(generateCCallPredef("v128WidenHighi16x8i8x16", 1), ValueType::v128);
                break;

            case Opcode::i16x8__widen_low_i8x16_u:
                pushExpression(generateCCallPredef("v128WidenLowi16x8u8x16", 1), ValueType::v128);
                break;

            case Opcode::i16x8__widen_high_i8x16_u:
                pushExpression(generateCCallPredef("v128WidenHighi16x8u8x16", 1), ValueType::v128);
                break;

            case Opcode::i16x8__shl:
                pushExpression(generateCCallPredef("v128Shli16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__shr_s:
                pushExpression(generateCCallPredef("v128Shri16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__shr_u:
                pushExpression(generateCCallPredef("v128Shru16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__add:
                pushExpression(generateCCallPredef("v128Addi16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__add_saturate_s:
                pushExpression(generateCCallPredef("v128SatAddi16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__add_saturate_u:
                pushExpression(generateCCallPredef("v128SatAddu16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__sub:
                pushExpression(generateCCallPredef("v128Subi16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__sub_saturate_s:
                pushExpression(generateCCallPredef("v128SatSubi16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__sub_saturate_u:
                pushExpression(generateCCallPredef("v128SatSubu16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__mul:
                pushExpression(generateCCallPredef("v128Muli16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__min_s:
                pushExpression(generateCCallPredef("v128Mini16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__min_u:
                pushExpression(generateCCallPredef("v128Minu16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__max_s:
                pushExpression(generateCCallPredef("v128Maxi16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__max_u:
                pushExpression(generateCCallPredef("v128Maxu16x8", 2), ValueType::v128);
                break;

            case Opcode::i16x8__avgr_u:
                pushExpression(generateCCallPredef("v128Avgru16x8", 2), ValueType::v128);
                break;

            case Opcode::i32x4__abs:
                pushExpression(generateCCallPredef("v128Absi32x4", 1), ValueType::v128);
                break;

            case Opcode::i32x4__neg:
                pushExpression(generateCCallPredef("v128Negi32x4", 1), ValueType::v128);
                break;

            case Opcode::i32x4__any_true:
                pushExpression(generateCCallPredef("v128SAnyTruei32x4", 1), ValueType::v128);
                break;

            case Opcode::i32x4__all_true:
                pushExpression(generateCCallPredef("v128SAllTruei32x4", 1), ValueType::v128);
                break;

            case Opcode::i32x4__widen_low_i16x8_s:
                pushExpression(generateCCallPredef("v128WidenLowi32x4i16x8", 1), ValueType::v128);
                break;

            case Opcode::i32x4__widen_high_i16x8_s:
                pushExpression(generateCCallPredef("v128WidenHighi32x4i16x8", 1), ValueType::v128);
                break;

            case Opcode::i32x4__widen_low_i16x8_u:
                pushExpression(generateCCallPredef("v128WidenLowi32x4u16x8", 1), ValueType::v128);
                break;

            case Opcode::i32x4__widen_high_i16x8_u:
                pushExpression(generateCCallPredef("v128WidenHighi32x4u16x8", 1), ValueType::v128);
                break;

            case Opcode::i32x4__shl:
                pushExpression(generateCCallPredef("v128Shli32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__shr_s:
                pushExpression(generateCCallPredef("v128Shri32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__shr_u:
                pushExpression(generateCCallPredef("v128Shru32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__add:
                pushExpression(generateCCallPredef("v128Addi32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__sub:
                pushExpression(generateCCallPredef("v128Subi32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__mul:
                pushExpression(generateCCallPredef("v128Muli32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__min_s:
                pushExpression(generateCCallPredef("v128Mini32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__min_u:
                pushExpression(generateCCallPredef("v128Minu32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__max_s:
                pushExpression(generateCCallPredef("v128Maxi32x4", 2), ValueType::v128);
                break;

            case Opcode::i32x4__max_u:
                pushExpression(generateCCallPredef("v128Maxu32x4", 2), ValueType::v128);
                break;

            case Opcode::i64x2__neg:
                pushExpression(generateCCallPredef("v128Negi64x2", 1), ValueType::v128);
                break;

            case Opcode::i64x2__shl:
                pushExpression(generateCCallPredef("v128Shli64x2", 2), ValueType::v128);
                break;

            case Opcode::i64x2__shr_s:
                pushExpression(generateCCallPredef("v128Shri64x2", 2), ValueType::v128);
                break;

            case Opcode::i64x2__shr_u:
                pushExpression(generateCCallPredef("v128Shru64x2", 2), ValueType::v128);
                break;

            case Opcode::i64x2__add:
                pushExpression(generateCCallPredef("v128Addi64x2", 2), ValueType::v128);
                break;

            case Opcode::i64x2__sub:
                pushExpression(generateCCallPredef("v128Subi64x2", 2), ValueType::v128);
                break;

            case Opcode::i64x2__mul:
                pushExpression(generateCCallPredef("v128Muli64x2", 2), ValueType::v128);
                break;

            case Opcode::f32x4__abs:
                pushExpression(generateCCallPredef("v128Absf32x4", 1), ValueType::v128);
                break;

            case Opcode::f32x4__neg:
                pushExpression(generateCCallPredef("v128Negf32x4", 1), ValueType::v128);
                break;

            case Opcode::f32x4__sqrt:
                pushExpression(generateCCallPredef("v128Sqrtf32x4", 1), ValueType::v128);
                break;

            case Opcode::f32x4__add:
                pushExpression(generateCCallPredef("v128Addf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__sub:
                pushExpression(generateCCallPredef("v128Subf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__mul:
                pushExpression(generateCCallPredef("v128Mulf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__div:
                pushExpression(generateCCallPredef("v128Divf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__min:
                pushExpression(generateCCallPredef("v128Minf32x4", 2), ValueType::v128);
                break;

            case Opcode::f32x4__max:
                pushExpression(generateCCallPredef("v128Maxf32x4", 2), ValueType::v128);
                break;

            case Opcode::f64x2__abs:
                pushExpression(generateCCallPredef("v128Absf64x2", 1), ValueType::v128);
                break;

            case Opcode::f64x2__neg:
                pushExpression(generateCCallPredef("v128Negf64x2", 1), ValueType::v128);
                break;

            case Opcode::f64x2__sqrt:
                pushExpression(generateCCallPredef("v128Sqrtf64x2", 1), ValueType::v128);
                break;

            case Opcode::f64x2__add:
                pushExpression(generateCCallPredef("v128Addf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__sub:
                pushExpression(generateCCallPredef("v128Subf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__mul:
                pushExpression(generateCCallPredef("v128Mulf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__div:
                pushExpression(generateCCallPredef("v128Divf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__min:
                pushExpression(generateCCallPredef("v128Minf64x2", 2), ValueType::v128);
                break;

            case Opcode::f64x2__max:
                pushExpression(generateCCallPredef("v128Maxf64x2", 2), ValueType::v128);
                break;

            case Opcode::i32x4__trunc_sat_f32x4_s:
                pushExpression(generateCCallPredef("satI32x4F32x4", 1), ValueType::v128);
                break;

            case Opcode::i32x4__trunc_sat_f32x4_u:
                pushExpression(generateCCallPredef("satU32x4F32x4", 1), ValueType::v128);
                break;

            case Opcode::f32x4__convert_i32x4_s:
                pushExpression(generateCCallPredef("convertF32x4I32x4", 1), ValueType::v128);
                break;

            case Opcode::f32x4__convert_i32x4_u:
                pushExpression(generateCCallPredef("convertF32x4U32x4", 1), ValueType::v128);
                break;

            default:
                std::cerr << "Unimplemented opcode '" << opcode << "' in generateCNode\n";
        }

        if (statement != nullptr) {
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

    if (kind != CNode::kIf && kind != CNode::kSwitch && kind != CNode::kLoop &&
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

void CGenerator::enhance()
{
    traverseStatements(function->getStatements(), [&](CNode* node) {
            if (auto* labelstatement = node->castTo<CLabel>(); labelstatement != nullptr) {
                labelMap[labelstatement->getLabel()].declaration = node;
            } else if (auto* branchStatement = node->castTo<CBranch>(); branchStatement != nullptr) {
                labelMap[branchStatement->getLabel()].useCount++;
            }
        });

    function->enhance(*this);
}

void CGenerator::generateC(std::ostream& os)
{
    if (enhanced) {
        enhance();
    }

    function->generateC(os, *this);
}

CGenerator::CGenerator(const Module* module, CodeEntry* codeEntry, bool enhanced)
  : module(module), codeEntry(codeEntry), enhanced(enhanced)
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
