// parser.h

#ifndef PARSER_H
#define PARSER_H

#include "Context.h"
#include "Encodings.h"
#include "TokenBuffer.h"

std::optional<ValueType> parseValueType(SourceContext& context);
std::optional<ElementType> parseElementType(SourceContext& context);
std::optional<uint32_t> parseTableIndex(SourceContext& context);
std::optional<uint32_t> parseFunctionIndex(SourceContext& context);
std::optional<uint32_t> parseMemoryIndex(SourceContext& context);
std::optional<uint32_t> parseGlobalIndex(SourceContext& context);
std::optional<uint32_t> parseLocalIndex(SourceContext& context);
std::optional<uint32_t> parseLabelIndex(SourceContext& context);
std::optional<ExternalKind> parseExternalKind(SourceContext& context);

bool startClause(SourceContext& context, std::string_view name);

uint32_t requiredU32(SourceContext& context);
int32_t requiredI32(SourceContext& context);
int64_t requiredI64(SourceContext& context);
float requiredF32(SourceContext& context);
double requiredF64(SourceContext& context);
std::string_view requiredString(SourceContext& context);
bool requiredParenthesis(SourceContext& context, char parenthesis);
bool requiredKeyword(SourceContext& context, std::string_view keyword);
bool requiredStartClause(SourceContext& context, std::string_view name);
std::optional<Limits> requiredLimits(SourceContext& context);

#endif
