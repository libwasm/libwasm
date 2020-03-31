// parser.cpp

#include "parser.h"

std::optional<ValueType> parseElementType(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getKeyword()) {
        if (auto encoding = ValueType::getEncoding(*value)) {
            if (*encoding == ValueType::funcref) {
                return encoding;
            }
        }
    }

    tokens.setPos(pos);
    return {};
}

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

std::optional<uint32_t> parseTypeIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getTypeCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getTypeIndex(*id); index != invalidIndex) {
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

std::optional<uint32_t> parseEventIndex(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value >= context.getEventCount()) {
            return {};
        }

        return *value;
    }

    if (auto id = tokens.getId()) {
        if (auto index = context.getEventIndex(*id); index != invalidIndex) {
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

std::optional<uint32_t> parseLane2Index(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value < 2) {
            return *value;
        }
    }

    return{};
}

std::optional<uint32_t> parseLane4Index(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value < 4) {
            return *value;
        }
    }

    return{};
}

std::optional<uint32_t> parseLane8Index(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value < 8) {
            return *value;
        }
    }

    return{};
}

std::optional<uint32_t> parseLane16Index(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value < 16) {
            return *value;
        }
    }

    return{};
}

std::optional<uint32_t> parseLane32Index(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU32()) {
        if (*value < 32) {
            return *value;
        }
    }

    return{};
}

std::optional<ExternalType> parseExternalType(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getKeyword()) {
        if (auto kind = ExternalType::getEncoding(*value)) {
            return kind;
        }
    }

    tokens.setPos(pos);
    return {};
}

std::optional<v128_t> parseV128(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();
    union
    {
        int8_t a8[16];
        int16_t a16[8];
        int32_t a32[4];
        int64_t a64[2];
        float aF[4];
        double aD[2];
        v128_t result;
    };

    if (auto id = tokens.getKeyword()) {
        bool ok = true;

        if (*id == "i8x16") {
            for (int i = 0; i < 16; ++i) {
                if (auto v  = tokens.getI8()) {
                    a8[i] = *v;
                } else {
                    ok = false;
                    break;
                }
            }
        } else if (*id == "i16x8") {
            for (int i = 0; i < 8; ++i) {
                if (auto v  = tokens.getI16()) {
                    a16[i] = *v;
                } else {
                    ok = false;
                    break;
                }
            }
        } else if (*id == "i32x4") {
            for (int i = 0; i < 4; ++i) {
                if (auto v  = tokens.getI32()) {
                    a32[i] = *v;
                } else {
                    ok = false;
                    break;
                }
            }
        } else if (*id == "i64x2") {
            for (int i = 0; i < 2; ++i) {
                if (auto v  = tokens.getI64()) {
                    a64[i] = *v;
                } else {
                    ok = false;
                    break;
                }
            }
        } else if (*id == "f32x4") {
            for (int i = 0; i < 4; ++i) {
                if (auto v  = tokens.getF32()) {
                    aF[i] = *v;
                } else {
                    ok = false;
                    break;
                }
            }
        } else if (*id == "f64x2") {
            for (int i = 0; i < 2; ++i) {
                if (auto v  = tokens.getF64()) {
                    aD[i] = *v;
                } else {
                    ok = false;
                    break;
                }
            }
        } else {
            ok =false;
        }

        if (ok) {
            return result;
        }
    }

    tokens.setPos(pos);
    return {};
}

int8_t requiredI8(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getI8()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "8-bit signed integer");
    return 0;
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

