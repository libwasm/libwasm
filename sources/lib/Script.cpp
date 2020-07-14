// Script.cpp

#include "Script.h"

#include "BackBone.h"
#include "Context.h"
#include "Module.h"
#include "common.h"
#include "parser.h"

#include <sstream>

using namespace std::string_literals;

namespace libwasm
{

static auto getNextResult()
{
    static unsigned resultCount = 0;

    return resultCount++;
}

static std::string makeResultName(unsigned resultNumber)
{
    return "result_" + toString(resultNumber);
}

static std::string makeExpectName(unsigned resultNumber)
{
    return "expect_" + toString(resultNumber);
}

ScriptValue::I8 ScriptValue::I8::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();
    I8 result;

    result.string = tokens.peekToken().getValue();

    if (auto v  = tokens.getI8()) {
        result.value = *v;
    } else {
        msgs.error(tokens.peekToken(), "Invalid I8");
        result.value = 0;
    }

    return result;
}

void ScriptValue::I8::generateC(std::ostream& os) const
{
    os << normalize(string);
}

static std::pair<std::string, std::string> makeNames(unsigned resultNumber, int lane = -1, const char* array = "")
{
    auto resultName = makeResultName(resultNumber);
    auto expectName = makeExpectName(resultNumber);

    if (lane >= 0) {
        resultName += "."s  + array + "[" + toString(uint32_t(lane)) + "]";
        expectName += "."s  + array + "[" + toString(uint32_t(lane)) + "]";
    }

    return std::make_pair(resultName, expectName);
}

void ScriptValue::I8::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane) const
{
    auto [resultName, expectName] = makeNames(resultNumber, lane, "i8");

    os << "\n\n    if (" << resultName << " != " << expectName << ") {"
          "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
          "\n        ++errorCount;"
          "\n    }";
}

ScriptValue::I16 ScriptValue::I16::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();
    I16 result;

    result.string = tokens.peekToken().getValue();

    if (auto v  = tokens.getI16()) {
        result.value = *v;
    } else {
        msgs.error(tokens.peekToken(), "Invalid I16");
        result.value = 0;
    }

    return result;
}

void ScriptValue::I16::generateC(std::ostream& os) const
{
    os << normalize(string);
}

void ScriptValue::I16::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane) const
{
    auto [resultName, expectName] = makeNames(resultNumber, lane, "i16");

    os << "\n\n    if (" << resultName << " != " << expectName << ") {"
          "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
          "\n        ++errorCount;"
          "\n    }";
}

ScriptValue::I32 ScriptValue::I32::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    I32 result;

    result.string = tokens.peekToken().getValue();
    result.value = requiredI32(context);

    return result;
}

void ScriptValue::I32::generateC(std::ostream& os) const
{
    os << normalize(string);
}

void ScriptValue::I32::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane) const
{
    auto [resultName, expectName] = makeNames(resultNumber, lane, "i32");

    os << "\n\n    if (" << resultName << " != " << expectName << ") {"
          "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
          "\n        ++errorCount;"
          "\n    }";
}

ScriptValue::I64 ScriptValue::I64::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    I64 result;

    result.string = tokens.peekToken().getValue();
    result.value = requiredI64(context);

    return result;
}

void ScriptValue::I64::generateC(std::ostream& os) const
{
    os << normalize(string) << "LL";
}

void ScriptValue::I64::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane) const
{
    auto [resultName, expectName] = makeNames(resultNumber, lane, "i64");

    os << "\n\n    if (" << resultName << " != " << expectName << ") {"
          "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
          "\n        ++errorCount;"
          "\n    }";
}

ScriptValue::F32 ScriptValue::F32::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    F32 result;

    result.string = tokens.peekToken().getValue();

    if (tokens.peekToken().getValue() == "nan:canonical") {
        result.nan = Nan::canonical;
        tokens.bump();
    } else if (tokens.peekToken().getValue() == "nan:arithmetic") {
        result.nan = Nan::arithmetic;
        tokens.bump();
    } else {
        result.value = requiredF32(context);
    }

    return result;
}

void ScriptValue::F32::generateC(std::ostream& os) const
{
    os << toString(toF32(string), true);
}

void ScriptValue::F32::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane) const
{
    auto [resultName, expectName] = makeNames(resultNumber, lane, "f32");
    auto* cast = "*(uint32_t*)&";
    auto* quietNan = "0x7fc00000U";

    if (nan == Nan::canonical) {
        os << "\n    if ((" << cast << resultName << " & " << quietNan << ") != " << quietNan << ") {"
            "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
            "\n        ++errorCount;"
            "\n    }";
    } else if (nan == Nan::arithmetic) {
        os << "\n    if (!isnan(" << resultName << ")) {"
            "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
            "\n        ++errorCount;"
            "\n    }";
    } else {
        os << "\n\n    if (" << cast << resultName << " != " << cast << expectName << ") {"
            "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
            "\n        ++errorCount;"
            "\n    }";
    }
}

ScriptValue::F64 ScriptValue::F64::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    F64 result;

    result.string = tokens.peekToken().getValue();

    if (tokens.peekToken().getValue() == "nan:canonical") {
        result.nan = Nan::canonical;
        tokens.bump();
    } else if (tokens.peekToken().getValue() == "nan:arithmetic") {
        result.nan = Nan::arithmetic;
        tokens.bump();
    } else {
        result.value = requiredF64(context);
    }

    return result;
}

void ScriptValue::F64::generateC(std::ostream& os) const
{
    os << toString(toF64(string), true);
}

void ScriptValue::F64::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane) const
{
    auto [resultName, expectName] = makeNames(resultNumber, lane, "f64");
    auto* cast = "*(uint64_t*)&";
    auto* quietNan = "0x7ff8000000000000ULL";

    if (nan == Nan::canonical) {
        os << "\n    if ((" << cast << resultName << " & " << quietNan << ") != " << quietNan << ") {"
            "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
            "\n        ++errorCount;"
            "\n    }";
    } else if (nan == Nan::arithmetic) {
        os << "\n    if (!isnan((float)" << resultName << ")) {"
            "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
            "\n        ++errorCount;"
            "\n    }";
    } else {
        os << "\n\n    if (" << cast << resultName << " != " << cast << expectName << ") {"
            "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
            "\n        ++errorCount;"
            "\n    }";
    }
}

ScriptValue::V128 ScriptValue::V128::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    V128 result;
    auto type = tokens.peekToken().getValue();

    tokens.bump();
    if (type == "i8x16") {
        result.type = ValueType::i8;
        result.count = 16;

        for (int i = 0; i < 16; ++i) {
            result.i8x16[i] = I8::parse(context);
        }
    } else if (type == "i16x8") {
        result.type = ValueType::i16;
        result.count = 8;

        for (int i = 0; i < 8; ++i) {
            result.i16x8[i] = I16::parse(context);
        }
    } else if (type == "i32x4") {
        result.type = ValueType::i32;
        result.count = 4;

        for (int i = 0; i < 4; ++i) {
            result.i32x4[i] = I32::parse(context);
        }
    } else if (type == "i64x2") {
        result.type = ValueType::i64;
        result.count = 2;

        for (int i = 0; i < 2; ++i) {
            result.i64x2[i] = I64::parse(context);
        }
    } else if (type == "f32x4") {
        result.type = ValueType::f32;
        result.count = 4;

        for (int i = 0; i < 4; ++i) {
            result.f32x4[i] = F32::parse(context);
        }
    } else if (type == "f64x2") {
        result.type = ValueType::f64;
        result.count = 2;

        for (int i = 0; i < 2; ++i) {
            result.f64x2[i] = F64::parse(context);
        }
    }

    return result;
}

void ScriptValue::V128::generateC(std::ostream& os) const
{
    const char* separator = "";

    switch(type) {
        case ValueType::i8:
            os << "v128Makei8x16(";

            for (unsigned i = 0; i < 16; ++i) {
                os << separator;
                separator = ", ";
                i8x16[i].generateC(os);
            }

            os << ')';
            break;

        case ValueType::i16:
            os << "v128Makei16x8(";

            for (unsigned i = 0; i < 8; ++i) {
                os << separator;
                separator = ", ";
                i16x8[i].generateC(os);
            }

            os << ')';
            break;

        case ValueType::i32:
            os << "v128Makei32x4(";

            for (unsigned i = 0; i < 4; ++i) {
                os << separator;
                separator = ", ";
                i32x4[i].generateC(os);
            }

            os << ')';
            break;

        case ValueType::i64:
            os << "v128Makei64x2(";

            for (unsigned i = 0; i < 2; ++i) {
                os << separator;
                separator = ", ";
                i64x2[i].generateC(os);
            }

            os << ')';
            break;

        case ValueType::f32:
            os << "v128Makef32x4(";

            for (unsigned i = 0; i < 4; ++i) {
                os << separator;
                separator = ", ";
                f32x4[i].generateC(os);
            }

            os << ')';
            break;

        case ValueType::f64:
            os << "v128Makef64x2(";

            for (unsigned i = 0; i < 2; ++i) {
                os << separator;
                separator = ", ";
                f64x2[i].generateC(os);
            }

            os << ')';
            break;

        default:
            break;
    }
}

void ScriptValue::V128::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber) const
{
    switch(type) {
        case ValueType::i8:
            for (unsigned i = 0; i < 16; ++i) {
                i8x16[i].generateAssert(os, lineNumber, resultNumber, i);
            }

            break;

        case ValueType::i16:
            for (unsigned i = 0; i < 8; ++i) {
                i16x8[i].generateAssert(os, lineNumber, resultNumber, i);
            }

            break;

        case ValueType::i32:
            for (unsigned i = 0; i < 4; ++i) {
                i32x4[i].generateAssert(os, lineNumber, resultNumber, i);
            }

            break;

        case ValueType::i64:
            for (unsigned i = 0; i < 2; ++i) {
                i64x2[i].generateAssert(os, lineNumber, resultNumber, i);
            }

            break;

        case ValueType::f32:
            for (unsigned i = 0; i < 4; ++i) {
                f32x4[i].generateAssert(os, lineNumber, resultNumber, i);
            }

            break;

        case ValueType::f64:
            for (unsigned i = 0; i < 2; ++i) {
                f64x2[i].generateAssert(os, lineNumber, resultNumber, i);
            }

            break;

        default:
            break;
    }
}

ScriptValue::ERef ScriptValue::ERef::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    ERef result;

    result.string = tokens.peekToken().getValue();
    result.value = requiredI64(context);

    return result;
}

void ScriptValue::ERef::generateC(std::ostream& os) const
{
    if (isNull) {
        os << "NULL";
    } else {
        os << "(_externalRefs + " << normalize(string) << "LL)";
    }
}

void ScriptValue::ERef::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane) const
{
    auto [resultName, expectName] = makeNames(resultNumber);

    os << "\n\n    if (" << resultName << " != " << expectName << ") {"
          "\n        fprintf(stderr, \"assert_return failed at line %d\\n\", " << lineNumber << ");"
          "\n        ++errorCount;"
          "\n    }";
}

ScriptValue ScriptValue::parse(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto& msgs = context.msgs();
    ScriptValue result;
    auto token = tokens.getKeyword();

    if (!token) {
        return result;
    }

    auto opcode = Opcode::fromString(*token);

    if (!opcode) {
        if (*token == "ref.extern") {
            result.type = ValueType::externref;
            result.eref = ERef::parse(context);
        }

        return result;
    }

    switch (*opcode) {
        case Opcode::i32__const:
            result.type = ValueType::i32;
            result.i32 = I32::parse(context);
            break;

        case Opcode::i64__const:
            result.type = ValueType::i64;
            result.i64 = I64::parse(context);
            break;

        case Opcode::f32__const:
            result.type = ValueType::f32;
            result.f32 = F32::parse(context);
            break;

        case Opcode::f64__const:
            result.type = ValueType::f64;
            result.f64 = F64::parse(context);
            break;

        case Opcode::v128__const:
            result.type = ValueType::v128;
            result.v128 = V128::parse(context);
            break;

        case Opcode::ref__null:
            (void) requiredRefType(context);
            result.type = ValueType::externref;
            result.eref.string = "0";
            result.eref.value = 0;
            result.eref.isNull = true;
            break;

        default:
            msgs.error(tokens.peekToken(-1), "Invalid instruction in assert");
    }

    return result;
}

void ScriptValue::generateC(std::ostream& os) const
{
    switch(type) {
        case ValueType::i32:
            i32.generateC(os);
            break;

        case ValueType::i64:
            i64.generateC(os);
            break;

        case ValueType::f32:
            f32.generateC(os);
            break;

        case ValueType::f64:
            f64.generateC(os);
            break;

        case ValueType::v128:
            v128.generateC(os);
            break;

        case ValueType::externref:
            eref.generateC(os);
            break;

        default:
            break;
    }
}

void ScriptValue::generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber) const
{
    switch(type) {
        case ValueType::i32:
            i32.generateAssert(os, lineNumber, resultNumber);
            break;

        case ValueType::i64:
            i64.generateAssert(os, lineNumber, resultNumber);
            break;

        case ValueType::f32:
            f32.generateAssert(os, lineNumber, resultNumber);
            break;

        case ValueType::f64:
            f64.generateAssert(os, lineNumber, resultNumber);
            break;

        case ValueType::v128:
            v128.generateAssert(os, lineNumber, resultNumber);
            break;

        case ValueType::externref:
            eref.generateAssert(os, lineNumber, resultNumber);
            break;

        default:
            break;
    }
}

Invoke* Invoke::parse(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (!startClause(context, "invoke")) {
        return nullptr;
    }

    auto result = new Invoke;

    if (auto id = tokens.getId()) {
        result->moduleName = *id;
    }

    result->functionName = requiredString(context);

    while (tokens.getParenthesis('(')) {
        result->arguments.push_back(ScriptValue::parse(context));
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
        result->results.push_back(ScriptValue::parse(context));
        requiredCloseParenthesis(context);
    }

    requiredCloseParenthesis(context);

    return result;
}

void AssertReturn::generateSimpleC(std::ostream& os, std::string_view type, const Script& script)
{
    auto resultNumber = getNextResult();

    auto resultName = makeResultName(resultNumber);
    auto expectName = makeExpectName(resultNumber);

    os << "\n\n    " << type << " " << resultName << " = ";
    if (type == "v128_u") {
        os << "(v128_u)";
    }

    invoke->generateC(os, script);
    os << ';';

    os << "\n    " << type << " " << expectName << " = ";
    if (type == "v128_u") {
        os << "(v128_u)";
    }

    results[0].generateC(os);
    os << ';';

    results[0].generateAssert(os, lineNumber, resultNumber);
}

void AssertReturn::generateMultiValueC(std::ostream& os, const Script& script)
{
    // TBI
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

        case ValueType::v128:
            return generateSimpleC(os, "v128_u", script);

        default:
            break;
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
          "\nunsigned errorCount = 0;"
          "\nextern void** _externalRefs;"
          "\nvoid spectest__initialize();";


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

            mainCode << "\n    " << cName(module->getId()) << "__initialize();";

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

    os << "\n    spectest__initialize();";
    os << mainCode.str();
    os << "\n    return errorCount != 0;"
        "\n}\n"
        "\n";
}

};
