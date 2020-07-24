// generateSimd.cpp

#include "common.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

using namespace std::string_literals;

std::stringstream hardwareMacros;
std::stringstream softwareDeclarations;
std::stringstream softwareDefinitions;

std::stringstream macros;
std::stringstream functionDeclarations;
std::stringstream functionDefinitions;

using namespace libwasm;

struct Type
{
    std::string_view type;
    uint32_t count;

    Type(std::string_view type, uint32_t count)
      : type(type), count(count)
    {
    }
};

static std::string makeFullType(std::string_view type, uint32_t count)
{
    std::string result(type.begin(), type.size());

    result += 'x';
    result += toString(count);

    return result;
}

template<typename... Types>
void generateOperations(void (*function)(std::string_view, std::string_view, uint32_t, std::string_view),
        std::string_view name, std::string_view op, Types... types)
{
    (function(name, types.type, types.count, op), ...);
}

static void generateMakeV128(std::string_view type, uint32_t count, std::string_view typeName,
         std::string_view parameterType)
{
    std::string fullType = makeFullType(type, count);
    std::string signature = "v128_t v128Make";

    signature += fullType;
    signature += '(';

    const char* seperator = "";

    for (uint32_t i = 0; i < count; ++i) {
        signature += seperator;
        signature += parameterType;
        signature += " v";
        signature += toString(i);
        seperator = ", ";
    }

    signature += ')';

    functionDeclarations << '\n' << signature << ';';

    functionDefinitions << '\n' << signature <<
        "\n{"
        "\n    v128_u result;"
        "\n";

    for (uint32_t i = 0; i < count; ++i) {
        functionDefinitions << "\n    result." << type << '[' << i << "] = ";

        if (typeName != parameterType) {
            functionDefinitions << '(' << typeName << ')';
        }

        functionDefinitions << "v" << i << ';';
    }

    functionDefinitions <<
        "\n"
        "\n    return result.v128;"
        "\n}\n";
}

static void generateBinaryOperationMacro(std::string_view name, std::string_view type, uint32_t count, std::string_view op)
{
    std::string fullType = makeFullType(type, count);
    
    hardwareMacros << "\n#define v128" << name << fullType << "(v1,v2) "
        "V(U(v1)." << fullType << " " << op << " U(v2)." << fullType << ')';

}

static void generateBinaryOperationFunction(std::string_view name, std::string_view type, uint32_t count, std::string_view op)
{
    std::string fullType = makeFullType(type, count);

    softwareDeclarations << "\nv128_t v128" << name << fullType << "(v128_t v1, v128_t v2);";

    softwareDefinitions << "\nv128_t v128" << name << fullType << "(v128_t v1, v128_t v2)"
        "\n{"
        "\n    v128_u result;"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << type << "[i] = ((U(v1))." << type << "[i]) " << op << " ((U(v2))." << type << "[i]);"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateBinaryOp(std::string_view name, std::string_view type, uint32_t count, std::string_view op)
{
    generateBinaryOperationMacro(name, type, count, op);
    generateBinaryOperationFunction(name, type, count, op);
}

static void generateUnaryOperationMacro(std::string_view name, std::string_view type, uint32_t count, std::string_view op)
{
    std::string fullType = makeFullType(type, count);
    
    hardwareMacros << "\n#define v128" << name << fullType << "(v1) "
        "V(" << op << "U(v1)." << fullType << ')';

}

static void generateUnaryOperationFunction(std::string_view name, std::string_view type, uint32_t count, std::string_view op)
{
    std::string fullType = makeFullType(type, count);

    softwareDeclarations << "\nv128_t v128" << name << fullType << "(v128_t v1);";

    softwareDefinitions << "\nv128_t v128" << name << fullType << "(v128_t v1)"
        "\n{"
        "\n    v128_u result;"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << type << "[i] = " << op << "((U(v1))." << type << "[i]);"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateUnaryOp(std::string_view name, std::string_view type, uint32_t count, std::string_view op)
{
    generateUnaryOperationMacro(name, type, count, op);
    generateUnaryOperationFunction(name, type, count, op);
}

static void generateBinaryCall(std::string_view name, std::string_view type, uint32_t count, std::string_view call)
{
    std::string fullType = makeFullType(type, count);

    functionDeclarations << "\nv128_t v128" << name << fullType << "(v128_t v1, v128_t v2);";

    functionDefinitions << "\nv128_t v128" << name << fullType << "(v128_t v1, v128_t v2)"
        "\n{"
        "\n    v128_u result;"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << type << "[i] = " << call << "(U(v1)." << type << "[i], U(v2)." << type << "[i]);"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateUnaryCall(std::string_view name, std::string_view type, uint32_t count, std::string_view call)
{
    std::string fullType = makeFullType(type, count);

    functionDeclarations << "\nv128_t v128" << name << fullType << "(v128_t v1);";

    functionDefinitions << "\nv128_t v128" << name << fullType << "(v128_t v1)"
        "\n{"
        "\n    v128_u result;"
        "\n    v128_u v1u = U(v1);"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << type << "[i] = " << call << "(v1u." << type << "[i]);"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateRelationalOp(std::string_view name, std::string_view type, uint32_t count,
        std::string_view op, std::string_view ones)
{
    std::string fullType = makeFullType(type, count);
    
    hardwareMacros << "\n#define v128" << name << fullType << "(v1,v2) "
        "V(U(v1)." << fullType << " " << op << " U(v2)." << fullType << ')';

    softwareDeclarations << "\nv128_t v128" << name << fullType << "(v128_t v1, v128_t v2);";

    auto resultType = type;

    if (resultType == "f32") {
        resultType = "i32";
    } else if (resultType == "f64") {
        resultType = "i64";
    }

    softwareDefinitions << "\nv128_t v128" << name << fullType << "(v128_t v1, v128_t v2)"
        "\n{"
        "\n    v128_u result;"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << resultType << "[i] = (U(v1)." << type << "[i] " << op <<
        " U(v2)." << type << "[i]) ? " << ones << " : 0;"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateRelationalOps(std::string_view name, std::string_view op)
{
    generateRelationalOp(name, "i8", 16, op, "0xff");
    generateRelationalOp(name, "u8", 16, op, "0xff");
    generateRelationalOp(name, "i16", 8, op, "0xffff");
    generateRelationalOp(name, "u16", 8, op, "0xffff");
    generateRelationalOp(name, "i32", 4, op, "0xffffffff");
    generateRelationalOp(name, "u32", 4, op, "0xffffffff");
    generateRelationalOp(name, "f32", 4, op, "0xffffffff");
    generateRelationalOp(name, "f64", 2, op, "0xffffffffffffffffLL");
}

static void generateShiftOp(std::string_view name, std::string_view type, uint32_t count,
        std::string_view op)
{
    std::string fullType = makeFullType(type, count);
    auto size = 128 / count;
    
    hardwareMacros << "\n#define v128" << name << fullType << "(v1,v2) "
        "V(U(v1)." << fullType << " " << op << " ((v2 % " << size << ")))";

    softwareDeclarations << "\nv128_t v128" << name << fullType << "(v128_t v1, int32_t v2);";

    softwareDefinitions << "\nv128_t v128" << name << fullType << "(v128_t v1, int32_t v2)"
        "\n{"
        "\n    v128_u result;"
        "\n    uint32_t shiftCount = v2 % " << size << ";"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << type << "[i] = ((U(v1))." << type << "[i]) " << op << " shiftCount;"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateSplat(std::string_view type, uint32_t count, std::string_view initType)
{
    std::string fullType = makeFullType(type, count);

    functionDeclarations << "\nv128_t v128Splat" << fullType << "(" << initType << " v1);";

    functionDefinitions << "\nv128_t v128Splat" << fullType << "(" << initType << " v1)"
        "\n{"
        "\n    v128_u result;"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << type << "[i] = v1;"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateLoadExtend(std::string_view type, uint32_t count)
{
    std::string fullType = makeFullType(type, count);

    functionDeclarations << "\nv128_t v128SLoadExt" << fullType << "(Memory* memory, uint64_t offset);";

    functionDefinitions << "\nv128_t v128SLoadExt" << fullType << "(Memory* memory, uint64_t offset)"
        "\n{"
        "\n    return v128Make" << fullType << '(';

    std::string capitalizedType;
    auto size = 8 / count;

    capitalizedType += toUpper(type[0]);
    capitalizedType += toString(size * 8);

    const char* seperator = "";
    for (uint32_t i = 0; i < count; ++i) {
        functionDefinitions << seperator << "load" << capitalizedType << "(memory, offset + " << (size * i) << ')';
        seperator = ", ";
    }

    functionDefinitions << ");"
        "\n}"
        "\n";
}

static void generateExtractLane(std::string_view type, uint32_t count)
{
    std::string fullType = makeFullType(type, count);

    macros << "\n#define v128ExtractLane" << fullType << "(v1,lane) (U(v1)." << type << "[lane])";
}

static void generateReplaceLane(std::string_view type, uint32_t count, std::string_view typeName)
{
    std::string fullType = makeFullType(type, count);

    functionDeclarations << "\nv128_t v128ReplaceLane" << fullType << "(v128_t v1, " << typeName << " v2, uint32_t lane);";

    functionDefinitions << "\nv128_t v128ReplaceLane" << fullType << "(v128_t v1, " << typeName << " v2, uint32_t lane)"
        "\n{"
        "\n    v128_u result = U(v1);"
        "\n"
        "\n    result." << type << "[lane] = v2;"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generateAnyTrue(std::string_view type, uint32_t count)
{
    std::string fullType = makeFullType(type, count);

    functionDeclarations << "\nint32_t v128SAnyTrue" << fullType << "(v128_t v1);";

    functionDefinitions << "\nint32_t v128SAnyTrue" << fullType << "(v128_t v1)"
        "\n{"
        "\n    v128_u v1u = U(v1);"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        if (v1u." << type << "[i] != 0) return 1;"
        "\n    }"
        "\n"
        "\n    return 0;"
        "\n}"
        "\n";
}

static void generateAllTrue(std::string_view type, uint32_t count)
{
    std::string fullType = makeFullType(type, count);

    functionDeclarations << "\nint32_t v128SAllTrue" << fullType << "(v128_t v1);";

    functionDefinitions << "\nint32_t v128SAllTrue" << fullType << "(v128_t v1)"
        "\n{"
        "\n    v128_u v1u = U(v1);"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        if (v1u." << type << "[i] == 0) return 0;"
        "\n    }"
        "\n"
        "\n    return 1;"
        "\n}"
        "\n";
}

static void generateWiden(std::string_view type, uint32_t count,
        std::string_view sourceType, bool high)
{
    std::string fullType = makeFullType(type, count);
    std::string sourceFullType = makeFullType(sourceType, count * 2);
    std::string signature = "v128_t v128Widen";

    signature += high ? "High" : "Low";
    signature += fullType;
    signature += sourceFullType;
    signature += "(v128_t v1)";

    functionDeclarations << '\n' << signature << ';';

    functionDefinitions << '\n' << signature <<
        "\n{"
        "\n    v128_u result;"
        "\n"
        "\n    for (uint32_t i = 0; i < " << count << "; ++i) {"
        "\n        result." << type << "[i] = (U(v1))." << sourceType << "[i";

    if (high) {
        functionDefinitions << " + " << count;
    }

    functionDefinitions << "];"
        "\n    }"
        "\n"
        "\n    return result.v128;"
        "\n}"
        "\n";
}

static void generate()
{
    generateMakeV128("i8", 16, "int8_t", "int32_t");
    generateMakeV128("u8", 16, "uint8_t", "uint32_t");
    generateMakeV128("i16", 8, "int16_t", "int32_t");
    generateMakeV128("u16", 8, "uint16_t", "uint32_t");
    generateMakeV128("i32", 4, "int32_t", "int32_t");
    generateMakeV128("u32", 4, "uint32_t", "uint32_t");
    generateMakeV128("i64", 2, "int64_t", "int64_t");
    generateMakeV128("u64", 2, "uint64_t", "uint64_t");
    generateMakeV128("f32", 4, "float", "float");
    generateMakeV128("f64", 2, "double", "double");

    generateLoadExtend("i16", 8);
    generateLoadExtend("u16", 8);
    generateLoadExtend("i32", 4);
    generateLoadExtend("u32", 4);
    generateLoadExtend("i64", 2);
    generateLoadExtend("u64", 2);

    generateOperations(generateBinaryOp, "Add", "+", Type("i8", 16), Type("i16", 8), Type("i32", 4),
            Type("i64", 2), Type("f32", 4), Type("f64", 2));

    generateOperations(generateBinaryOp, "Sub", "-", Type("i8", 16), Type("i16", 8), Type("i32", 4),
            Type("i64", 2), Type("f32", 4), Type("f64", 2));

    generateOperations(generateBinaryOp, "Mul", "*", Type("i8", 16), Type("i16", 8), Type("i32", 4),
            Type("i64", 2), Type("f32", 4), Type("f64", 2));

    generateBinaryOp("Div", "f32", 4, "/");
    generateBinaryOp("Div", "f64", 2, "/");

    generateBinaryOp("Or",  "i64", 2, "|");
    generateBinaryOp("And", "i64", 2, "&");
    generateBinaryOp("Xor", "i64", 2, "^");

    generateOperations(generateUnaryOp, "Neg", "-", Type("i8", 16), Type("i16", 8), Type("i32", 4),
            Type("i64", 2), Type("f32", 4), Type("f64", 2));

    generateUnaryOp("Not", "i64", 2, "~");

    generateBinaryCall("Avgr", "u8", 16, "AVGR");
    generateBinaryCall("Avgr", "u16", 8, "AVGR");

    generateOperations(generateBinaryCall, "Max", "MAX_VALUE", Type("i8", 16), Type("i16", 8), Type("i32", 4),
            Type("i64", 2), Type("u8", 16), Type("u16", 8), Type("u32", 4), Type("u64", 2));

    generateBinaryCall("Max", "f32", 4, "maxF32");
    generateBinaryCall("Max", "f64", 2, "maxF64");

    generateOperations(generateBinaryCall, "Min", "MIN_VALUE", Type("i8", 16), Type("i16", 8), Type("i32", 4),
            Type("i64", 2), Type("u8", 16), Type("u16", 8), Type("u32", 4), Type("u64", 2));

    generateBinaryCall("Min", "f32", 4, "minF32");
    generateBinaryCall("Min", "f64", 2, "minF64");

    generateUnaryCall("Abs", "i8", 16, "ABS_VALUE");
    generateUnaryCall("Abs", "i16", 8, "ABS_VALUE");
    generateUnaryCall("Abs", "i32", 4, "ABS_VALUE");

    generateBinaryCall("SatAdd", "i8", 16, "SatAddi8");
    generateBinaryCall("SatAdd", "u8", 16, "SatAddu8");
    generateBinaryCall("SatAdd", "i16", 8, "SatAddi16");
    generateBinaryCall("SatAdd", "u16", 8, "SatAddu16");

    generateBinaryCall("SatSub", "i8", 16, "SatSubi8");
    generateBinaryCall("SatSub", "u8", 16, "SatSubu8");
    generateBinaryCall("SatSub", "i16", 8, "SatSubi16");
    generateBinaryCall("SatSub", "u16", 8, "SatSubu16");

    generateUnaryCall("Sqrt", "f32", 4, "sqrtf");
    generateUnaryCall("Sqrt", "f64", 2, "sqrt");

    generateUnaryCall("Abs", "f32", 4, "fabsf");
    generateUnaryCall("Abs", "f64", 2, "fabs");

    generateRelationalOps("Eq", "==");
    generateRelationalOps("Ne", "!=");
    generateRelationalOps("Gt", ">");
    generateRelationalOps("Lt", "<");
    generateRelationalOps("Ge", ">=");
    generateRelationalOps("Le", "<=");

    generateShiftOp("Shl", "i8", 16, "<<");
    generateShiftOp("Shr", "i8", 16, ">>");
    generateShiftOp("Shr", "u8", 16, ">>");
    generateShiftOp("Shl", "i16", 8, "<<");
    generateShiftOp("Shr", "i16", 8, ">>");
    generateShiftOp("Shr", "u16", 8, ">>");
    generateShiftOp("Shl", "i32", 4, "<<");
    generateShiftOp("Shr", "i32", 4, ">>");
    generateShiftOp("Shr", "u32", 4, ">>");
    generateShiftOp("Shl", "i64", 2, "<<");
    generateShiftOp("Shr", "i64", 2, ">>");
    generateShiftOp("Shr", "u64", 2, ">>");

    generateSplat("i8", 16, "int32_t");
    generateSplat("i16", 8, "int32_t");
    generateSplat("i32", 4, "int32_t");
    generateSplat("i64", 2, "int64_t");
    generateSplat("f32", 4, "float");
    generateSplat("f64", 2, "double");

    generateExtractLane("i8", 16);
    generateExtractLane("u8", 16);
    generateExtractLane("i16", 8);
    generateExtractLane("u16", 8);
    generateExtractLane("i32", 4);
    generateExtractLane("i64", 2);
    generateExtractLane("f32", 4);
    generateExtractLane("f64", 2);

    generateReplaceLane("i8", 16, "int32_t");
    generateReplaceLane("i16", 8, "int32_t");
    generateReplaceLane("i32", 4, "int32_t");
    generateReplaceLane("i64", 2, "int64_t");
    generateReplaceLane("f32", 4, "float");
    generateReplaceLane("f64", 2, "double");

    generateAnyTrue("i8", 16);
    generateAnyTrue("i16", 8);
    generateAnyTrue("i32", 4);

    generateAllTrue("i8", 16);
    generateAllTrue("i16", 8);
    generateAllTrue("i32", 4);

    generateWiden("i16", 8, "i8", false);
    generateWiden("i16", 8, "i8", true);
    generateWiden("i16", 8, "u8", false);
    generateWiden("i16", 8, "u8", true);
    generateWiden("i32", 4, "i16", false);
    generateWiden("i32", 4, "i16", true);
    generateWiden("i32", 4, "u16", false);
    generateWiden("i32", 4, "u16", true);
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " declaration-file definition-file\n";
        return 1;
    }

    generate();

    std::ofstream hFile(argv[1]);

    hFile << "// " << argv[1] << " generated by generateSimd."
        "\n";

    hFile.write(functionDeclarations.str().data(), functionDeclarations.str().size());
    hFile << '\n';

    hFile.write(macros.str().data(), macros.str().size());
    hFile << '\n';

    hFile << "\n#ifdef HARDWARE_SUPPORT";

    hFile.write(hardwareMacros.str().data(), hardwareMacros.str().size());

    hFile << "\n#else";

    hFile.write(softwareDeclarations.str().data(), softwareDeclarations.str().size());

    hFile << "\n#endif"
        "\n";

    std::ofstream cFile(argv[2]);

    cFile << "// " << argv[2] << " generated by generateSimd."
        "\n"
        "\n#ifndef HARDWARE_SUPPORT";

    cFile.write(softwareDefinitions.str().data(), softwareDefinitions.str().size());

    cFile << "\n#endif"
        "\n";

    cFile.write(functionDefinitions.str().data(), functionDefinitions.str().size());
    cFile << '\n';
}

