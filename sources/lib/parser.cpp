// parser.cpp

#include "parser.h"

std::optional<ValueType> parseValueType(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getKeyword()) {
        if (auto encoding = ValueType::getEncoding(*value)) {
            return encoding;
        }
    }

    tokens.setPos(pos);
    return {};
}

std::optional<uint32_t> parseTableIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getTableCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getTableIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    return{};
}

std::optional<uint32_t> parseFunctionIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getFunctionCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getFunctionIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    return{};
}

std::optional<uint32_t> parseMemoryIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getMemoryCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getMemoryIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    return{};
}

std::optional<uint32_t> parseGlobalIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getGlobalCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getGlobalIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    return{};
}

std::optional<uint32_t> parseLocalIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getLocalCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getLocalIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    return{};
}

std::optional<uint32_t> parseLabelIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getLabelCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getLabelIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    return{};
}

std::optional<ExternalKind> parseExternalKind(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getKeyword()) {
        if (auto kind = ExternalKind::getEncoding(*value)) {
            return kind;
        }
    }

    tokens.setPos(pos);
    return {};
}

uint32_t requiredU32(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "32-bit unsigned integer");
    return 0;
}

int32_t requiredI32(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getI32()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "32-bit signed integer");
    return 0;
}

int64_t requiredI64(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getI64()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "64-bit signed integer");
    return 0;
}

float requiredF32(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getF32()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "32-bit floating pointer number");
    return 0;
}

double requiredF64(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getF64()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "64-bit floating pointer number");
    return 0;
}

std::string_view requiredString(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getString()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "name");
    return {};
}

bool requiredParenthesis(SourceContext& context, char parenthesis)
{
    auto& tokens = context.tokens();

    if (!tokens.getParenthesis(parenthesis)) {
        context.msgs().expected(tokens.peekToken(), '\'', parenthesis, '\'');
        return false;
    }

    return true;
}

bool requiredKeyword(SourceContext& context, std::string_view keyword)
{
    auto& tokens = context.tokens();

    if (!tokens.getKeyword(keyword)) {
        context.msgs().expected(tokens.peekToken(), '\'', keyword, '\'');
        return false;
    }

    return true;
}

bool startClause(SourceContext& context, std::string_view name)
{
    auto& tokens = context.tokens();

    if (tokens.peekParenthesis('(') && tokens.peekKeyword(name, 1)) {
        tokens.bump(2);
        return true;
    }

    return false;
}

bool requiredStartClause(SourceContext& context, std::string_view name)
{
    return requiredParenthesis(context, '(') && requiredKeyword(context, name);
}

std::optional<Limits> requiredLimits(SourceContext& context)
{
    auto& tokens = context.tokens();
    Limits result;

    result.min = requiredU32(context);

    if (auto max = tokens.getU32()) {
        result.max = *max;
        result.kind = LimitKind::hasMax;
    }

    return result;
}

