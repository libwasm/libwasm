// Script.h

#ifndef SCRIPT_H
#define SCRIPT_H

#include "Encodings.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace libwasm
{

class Module;
class SourceContext;
class Invoke;
class Script;
class AssertReturn;

class ScriptValue
{
    public:
        void generateC(std::ostream& os) const;
        static ScriptValue parse(SourceContext& context);

        enum Nan : uint8_t
        {
            none,
            canonical,
            arithmetic
        };

        struct I8
        {
            bool operator==(uint8_t other)
            {
                return value == other;
            }

            void generateC(std::ostream& os) const;
            static I8 parse(SourceContext& context);

            uint8_t value;
            std::string_view string;
        };

        struct I16
        {
            bool operator==(uint16_t other)
            {
                return value == other;
            }

            void generateC(std::ostream& os) const;
            static I16 parse(SourceContext& context);

            uint16_t value;
            std::string_view string;
        };

        struct I32
        {
            bool operator==(uint32_t other)
            {
                return value == other;
            }

            void generateC(std::ostream& os) const;
            static I32 parse(SourceContext& context, bool forV128 = false);

            uint32_t value;
            std::string_view string;
        };

        struct I64
        {
            bool operator==(uint64_t other)
            {
                return value == other;
            }

            void generateC(std::ostream& os) const;
            static I64 parse(SourceContext& context, bool forV128 = false);

            uint64_t value;
            std::string_view string;
        };

        struct F32
        {
            bool operator==(float other);
            void generateC(std::ostream& os) const;
            static F32 parse(SourceContext& context, bool forV128 = false);

            Nan nan = Nan::none;
            float value;
            std::string_view string;
        };

        struct F64
        {
            bool operator==(double other);
            void generateC(std::ostream& os) const;
            static F64 parse(SourceContext& context, bool forV128 = false);

            Nan nan = Nan::none;
            double value;
            std::string_view string;
        };

        struct V128
        {
            bool operator==(V128 other);
            void generateC(std::ostream& os) const;
            static F64 parse(SourceContext& context);

            union {
                I8  i8x16[16];
                I16 i16x8[8];
                I32 i32x4[4];
                I64 i64x2[2];
                F32 f32x4[4];
                F64 f64x2[2];
            };

            std::string_view string;
        };

        union
        {
            int32_t  i32;
            int64_t  i64;
            float    f32;
            double   f64;
            int8_t   i8x16[16];
            int16_t  i16x8[8];
            int32_t  i32x4[4];
            int64_t  i64x2[2];
            float    f32x4[4];
            double   f64x2[2];
            v128_t   v128;
        };

        ValueType type = ValueType::void_;
        Nan nan = Nan::none;
        Nan nans[4];

        std::string_view string;
};

class Invoke
{
    public:
        Invoke() = default;

        void generateC(std::ostream& os, const Script& script);
        static Invoke* parse(SourceContext& context);


    private:
        std::string moduleName;
        std::string functionName;
        std::vector<ScriptValue> arguments;
};

class AssertReturn
{
    public:
        AssertReturn() = default;

        void generateC(std::ostream& os, const Script& script);
        void generateSimpleC(std::ostream& os, std::string_view type, const Script& script);
        void generateMultiValueC(std::ostream& os, const Script& script);
        void generateV128C(std::ostream& os, const Script& script);
        static AssertReturn* parse(SourceContext& context);

    private:
        Invoke* invoke;
        std::vector<ScriptValue> results;
        size_t lineNumber = 0;
};


class Script
{
    public:
        Script() = default;

        void addModule(std::shared_ptr<Module>& module);
        void addAssertReturn(std::shared_ptr<AssertReturn>& assertReturn);
        void addInvoke(std::shared_ptr<Invoke>& invoke);

        const auto* getLastModule() const
        {
            return lastModule;
        }

        void generateC(std::ostream& os, bool optimized);

    private:
        struct Command
        {
            Command(std::shared_ptr<Module>& module)
              : module(module)
            {
            }

            Command(std::shared_ptr<AssertReturn>& assertReturn)
              : assertReturn(assertReturn)
            {
            }

            Command(std::shared_ptr<Invoke>& invoke)
              : invoke(invoke)
            {
            }

            std::shared_ptr<Module> module;
            std::shared_ptr<AssertReturn> assertReturn;
            std::shared_ptr<Invoke> invoke;
        };

        const Module* lastModule = nullptr;
        std::vector<Command> commands;
};

};

#endif
