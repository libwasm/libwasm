// Script.cpp

#include "Script.h"

#include "Context.h"
#include "parser.h"

namespace libwasm
{

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
            result.type = ValueType::i32;
            result.i32 = requiredI32(context);
            break;

        case Opcode::i64__const:
            result.type = ValueType::i64;
            result.i64 = requiredI64(context);
            break;

        case Opcode::f32__const:
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

    result->invoke = invoke;

    while (tokens.getParenthesis('(')) {
        result->results.push_back(Invoke::Value::parse(context));
        requiredCloseParenthesis(context);
    }

    requiredCloseParenthesis(context);

    return result;
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

};
