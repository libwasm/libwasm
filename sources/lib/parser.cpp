// parser.cpp

#include "parser.h"
#include "Module.h"


namespace libwasm
{
std::optional<ValueType> parseElementType(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getKeyword()) {
        if (auto encoding = ValueType::getEncoding(*value)) {
            if (encoding->isValidRef()) {
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

std::optional<ValueType> parseRefType(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getKeyword()) {
        if (auto encoding = ValueType::getRefEncoding(*value)) {
            return encoding;
        }
    }

    tokens.setPos(pos);
    return {};
}

std::optional<uint32_t> parseTableIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getTableCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getTableIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseTypeIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getTypeCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getTypeIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseFunctionIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getFunctionCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getFunctionIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseMemoryIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getMemoryCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getMemoryIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseEventIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getEventCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getEventIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseSegmentIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getSegmentCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getSegmentIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseElementIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getElementCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getElementIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseGlobalIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getGlobalCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getGlobalIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseLocalIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < module->getLocalCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getLocalIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseLabelIndex(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto* module = context.getModule();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value <= module->getLabelCount()) {
            return *value;
        }
    } else if (auto id = context.getId()) {
        if (auto index = module->getLabelIndex(*id); index != invalidIndex) {
            return index;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseLane2Index(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < 2) {
            return *value;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseLane4Index(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < 4) {
            return *value;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseLane8Index(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < 8) {
            return *value;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseLane16Index(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < 16) {
            return *value;
        }
    }

    tokens.setPos(pos);
    return{};
}

std::optional<uint32_t> parseLane32Index(SourceContext& context)
{
    auto& tokens = context.tokens();
    auto pos = tokens.getPos();

    if (auto value = tokens.getU32()) {
        if (*value < 32) {
            return *value;
        }
    }

    tokens.setPos(pos);
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

uint8_t requiredU8(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getU8()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "8-bit unsigned integer");
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

    context.msgs().expected(tokens.peekToken(), "32-bit floating point number");
    return 0;
}

double requiredF64(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = tokens.getF64()) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "64-bit floating point number");
    return 0;
}

v128_t requiredV128(SourceContext context)
{
    auto& tokens = context.tokens();

    if (auto value = parseV128(context)) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "128-bit vector number");
    return {};
}

std::string requiredString(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = context.getString()) {
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
        result.flags = Limits::hasMaxFlag;
    }

    if (tokens.getKeyword("shared")) {
        result.flags |= Limits::isSharedFlag;
    }

    return result;
}

bool requiredCloseParenthesis(SourceContext& context)
{
    if (requiredParenthesis(context, ')')) {
        return true;
    } else {
        context.tokens().recover();
        return false;
    }
}

ValueType requiredValueType(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = parseValueType(context)) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "value type");
    return {};
}

ValueType requiredRefType(SourceContext& context)
{
    auto& tokens = context.tokens();

    if (auto value = parseRefType(context)) {
        return *value;
    }

    context.msgs().expected(tokens.peekToken(), "value type");
    return {};
}

};
