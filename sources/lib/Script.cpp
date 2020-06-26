// Script.cpp

#include "Script.h"

#include "BackBone.h"
#include "Context.h"
#include "Module.h"
#include "common.h"
#include "parser.h"

#include <sstream>

namespace libwasm
{

auto getNextResult()
{
    static unsigned resultCount = 0;

    return resultCount++;
}

Invoke::Value Invoke::Value::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();
    Value result;
    auto token = tokens.getKeyword();

    if (!token) {
        return result;
    }

    auto opcode = Opcode::fromString(*token);

    if (!opcode) {
        return result;
    }

    switch (*opcode) {
        case Opcode::i32__const:
            result.string = tokens.peekToken().getValue();
            result.type = ValueType::i32;
            result.i32 = requiredI32(context);
            break;

        case Opcode::i64__const:
            result.string = tokens.peekToken().getValue();
            result.type = ValueType::i64;
            result.i64 = requiredI64(context);
            break;

        case Opcode::f32__const:
            result.string = tokens.peekToken().getValue();
            result.type = ValueType::f32;

            if (tokens.peekToken().getValue() == "nan:canonical") {
                result.canonicalNan = true;
                tokens.bump();
            } else if (tokens.peekToken().getValue() == "nan:arithmetic") {
                result.arithmeticNan = true;
                tokens.bump();
            } else {
                result.f32 = requiredF32(context);
            }

            break;

        case Opcode::f64__const:
            result.string = tokens.peekToken().getValue();
            result.type = ValueType::f64;

            if (tokens.peekToken().getValue() == "nan:canonical") {
                result.canonicalNan = true;
                tokens.bump();
            } else if (tokens.peekToken().getValue() == "nan:arithmetic") {
                result.arithmeticNan = true;
                tokens.bump();
            } else {
                result.f64 = requiredF64(context);
            }

            break;

        case Opcode::v128__const:
            result.type = ValueType::v128;
            result.v128 = requiredV128(context);
            break;

        default:
            msgs.error(tokens.peekToken(-1), "Invalid instruction in assert");
    }

    return result;
}

static bool isSpecialF(std::string_view string)
{
    if (string[0] == 'i' || string[0] == 'n') {
        return true;
    }

    if (string[0] == '+' || string[0] == '-') {
        if (string[1] == 'i' || string[1] == 'n') {
            return true;
        }
    }

    return false;
}

void Invoke::Value::generateC(std::ostream& os) const
{
    switch(type) {
        case ValueType::i32:
            os << normalize(string);
            break;

        case ValueType::i64:
            os << normalize(string) << "LL";
            break;

        case ValueType::f32:
            os << toString(toF32(string), true);
            break;

        case ValueType::f64:
            os << toString(toF64(string), true);
            break;

        case ValueType::v128:
            os << "v128Makei64x2(" << i64x2[0] << ", " << i64x2[1] << ')';
            break;
    }
}

Invoke* Invoke::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();

    if (!startClause(context, "invoke")) {
        return nullptr;
    }

    auto result = new Invoke;

    if (auto id = tokens.getId()) {
        result->moduleName = *id;
    }

    result->functionName = requiredString(context);

    while (tokens.getParenthesis('(')) {
        result->arguments.push_back(Value::parse(context));
        requiredCloseParenthesis(context);
    }

    requiredCloseParenthesis(context);

    return result;
}

void Invoke::generateC(std::ostream& os, const Script& script)
{
    if (moduleName.empty()) {
        moduleName = script.getLastModule()->getId();
    }

    os << cName(moduleName) << "__" << cName(functionName) << '(';

    const char* separator = "";

    for (const auto& argument : arguments) {
        os << separator;
        argument.generateC(os);
        separator = ", ";
    }

    os << ')';
}

AssertReturn* AssertReturn::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "assert_return")) {
        return nullptr;
    }

    auto* invoke = Invoke::parse(context);

    if (invoke == nullptr) {
        tokens.recover();
        return nullptr;
    }

    auto result = new AssertReturn;

    result->lineNumber = tokens.peekToken().getLineNumber();
    result->invoke = invoke;

    while (tokens.getParenthesis('(')) {
        result->results.push_back(Invoke::Value::parse(context));
        requiredCloseParenthesis(context);
    }

    requiredCloseParenthesis(context);

    return result;
}

void AssertReturn::generateSimpleC(std::ostream& os, std::string_view type, const Script& script)
{
    auto resultNumber = getNextResult();
    std::string cast;
    std::string quietNan;
    std::string quietNegNan;

    if (type == "float") {
        cast = "*(uint32_t*)&";
        quietNan = "0x7fc00000U";
        quietNegNan = "0xffc00000U";
    } else if (type == "double") {
        cast = "*(uint64_t*)&";
        quietNan = "0x7ff8000000000000ULL";
        quietNegNan = "0xfff8000000000000ULL";
    }

    os << "\n\n    " << type << " result_" << resultNumber << " = ";
    invoke->generateC(os, script);
    os << ';';

    if (results[0].canonicalNan) {
        os << "\n    if (" << cast << "result_" << resultNumber << " != " << quietNan << " && " <<
            cast << "result_" << resultNumber << " != " << quietNegNan << ") {";
        os << "\n        printf(\"assert_return failed at line %d\\n\", " << lineNumber << ");";
        os << "\n        ++errorCount;";
        os << "\n    }";
    } else if (results[0].arithmeticNan) {
        os << "\n    if (!isnan(result_" << resultNumber << ")) {";
        os << "\n        printf(\"assert_return failed at line %d\\n\", " << lineNumber << ");";
        os << "\n        ++errorCount;";
        os << "\n    }";
    } else {
        os << "\n    " << type << " expect_" << resultNumber << " = ";
        results[0].generateC(os);
        os << ';';

        os << "\n\n    if (" << cast << "result_" << resultNumber << " != " << cast << "expect_" << resultNumber << ") {";
        os << "\n        printf(\"assert_return failed at line %d\\n\", " << lineNumber << ");";
        os << "\n        ++errorCount;";
        os << "\n    }";
    }
}

void AssertReturn::generateMultiValueC(std::ostream& os, const Script& script)
{
    // TBI
}

void AssertReturn::generateV128C(std::ostream& os, const Script& script)
{
    auto resultNumber = getNextResult();

    os << "\n\n    v128_u result_" << resultNumber << " = (v128_u)";
    invoke->generateC(os, script);
    os << ';';

    os << "\n    v128_u expect_" << resultNumber << " = (v128_u)";
    results[0].generateC(os);
    os << ';';

    os << "\n\n    if (result_" << resultNumber << ".u64[0] != expect_" << resultNumber << ".u64[0] && "
        "result_" << resultNumber << ".u64[0] != expect_" << resultNumber << ".u64[0]) {" ;
    os << "\n        printf(\"assert_return failed at line %d\\n\", " << lineNumber << ");";
    os << "\n    }";
}

void AssertReturn::generateC(std::ostream& os, const Script& script)
{
    if (results.empty()) {
        os << "\n    ";
        invoke->generateC(os, script);
        os << ';';
        return;
    }

    if (results.size() > 1) {
        return generateMultiValueC(os, script);
    }

    switch (results[0].type) {
        case ValueType::i32:
            return generateSimpleC(os, "int32_t", script);

        case ValueType::i64:
            return generateSimpleC(os, "int64_t", script);

        case ValueType::f32:
            return generateSimpleC(os, "float", script);

        case ValueType::f64:
            return generateSimpleC(os, "double", script);

        default:
            return generateV128C(os, script);
    }
}

void Script::addModule(std::shared_ptr<Module>& module)
{
    commands.emplace_back(module);
}

void Script::addAssertReturn(std::shared_ptr<AssertReturn>& assertReturn)
{
    commands.emplace_back(assertReturn);
}

void Script::addInvoke(std::shared_ptr<Invoke>& invoke)
{
    commands.emplace_back(invoke);
}

void Script::generateC(std::ostream& os, bool optimized)
{
    os << "\n#include \"libwasm.h\""
          "\n"
          "\n#include <stdint.h>"
          "\n#include <math.h>"
          "\n#include <string.h>"
          "\n"
          "\nunsigned errorCount = 0;";

    std::stringstream mainCode;

    unsigned moduleNameCount = 0;

    for (auto& command : commands) {
        if (command.module) {
            auto& module = command.module;

            if (module->getId().empty()) {
                module->setId("module_" + toString(moduleNameCount++));
            }

            module->generateC(os, optimized);
            lastModule = module.get();

            mainCode << "\n    " << module->getId() << "__initialize();";

            if (auto* exportSection = module->getExportSection(); exportSection != nullptr) {
                for (auto& export_ : exportSection->getExports()) {
                    auto index = export_->getIndex();

                    os << "\n#define " << module->getId() << "__" << cName(export_->getName()) << ' ';

                    switch (export_->getKind()) {
                        case ExternalType::function:
                            os << module->getFunction(index)->getCName(module.get());
                            break;

                        case ExternalType::table:
                            os << module->getTable(index)->getCName(module.get());
                            break;

                        case ExternalType::memory:
                            os << module->getMemory(index)->getCName(module.get());
                            break;

                        case ExternalType::global:
                            os << module->getGlobal(index)->getCName(module.get());
                            break;

                        case ExternalType::event:
                            break;

                        default:
                            break;
                    }
                }

                os << '\n';
            }
        } else if (command.invoke) {
            mainCode << "\n    ";
            command.invoke->generateC(mainCode, *this);
            mainCode << ';';
        } else if (command.assertReturn) {
            command.assertReturn->generateC(mainCode, *this);
        }
    }

    os << "\n\nint main()"
        "\n{";

    os << mainCode.str();
    os << "\n    return errorCount != 0;";
    os << "\n}\n";
}

};
