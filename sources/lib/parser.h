// parser.h

#ifndef PARSER_H
#define PARSER_H

#include "Context.h"
#include "Encodings.h"
#include "TokenBuffer.h"

namespace libwasm
{

std::optional<ValueType> parseValueType(SourceContext& context);
std::optional<ValueType> parseElementType(SourceContext& context);
std::optional<uint32_t> parseTableIndex(SourceContext& context);
std::optional<uint32_t> parseTypeIndex(SourceContext& context);
std::optional<uint32_t> parseFunctionIndex(SourceContext& context);
std::optional<uint32_t> parseMemoryIndex(SourceContext& context);
std::optional<uint32_t> parseGlobalIndex(SourceContext& context);
std::optional<uint32_t> parseElementIndex(SourceContext& context);
std::optional<uint32_t> parseSegmentIndex(SourceContext& context);
std::optional<uint32_t> parseLocalIndex(SourceContext& context);
std::optional<uint32_t> parseLabelIndex(SourceContext& context);
std::optional<uint32_t> parseEventIndex(SourceContext& context);
std::optional<uint32_t> parseLane2Index(SourceContext& context);
std::optional<uint32_t> parseLane4Index(SourceContext& context);
std::optional<uint32_t> parseLane8Index(SourceContext& context);
std::optional<uint32_t> parseLane16Index(SourceContext& context);
std::optional<uint32_t> parseLane32Index(SourceContext& context);
std::optional<v128_t> parseV128(SourceContext& context);
std::optional<ExternalType> parseExternalType(SourceContext& context);

bool startClause(SourceContext& context, std::string_view name);

int8_t requiredI8(SourceContext& context);
uint8_t requiredU8(SourceContext& context);
uint32_t requiredU32(SourceContext& context);
int32_t requiredI32(SourceContext& context);
int64_t requiredI64(SourceContext& context);
float requiredF32(SourceContext& context);
double requiredF64(SourceContext& context);
v128_t requiredV128(SourceContext context);
std::string_view requiredString(SourceContext& context);
bool requiredParenthesis(SourceContext& context, char parenthesis);
bool requiredKeyword(SourceContext& context, std::string_view keyword);
bool requiredStartClause(SourceContext& context, std::string_view name);
std::optional<Limits> requiredLimits(SourceContext& context);
bool requiredCloseParenthesis(SourceContext& context);

};

#endif
