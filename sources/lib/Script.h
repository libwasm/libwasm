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

class Invoke
{
    public:
        Invoke() = default;

        void generateC(std::ostream& os);
        static Invoke* parse(SourceContext& context);

        struct Value
        {
            ValueType type = ValueType::void_;
            bool canonicalNan = false;
            bool arithmeticNan = false;
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

            static Value parse(SourceContext& context);
        };

    private:
        std::string moduleName;
        std::string functionName;
        std::vector<Value> arguments;
};

class AssertReturn
{
    public:
        AssertReturn() = default;

        void generateC(std::ostream& os);
        static AssertReturn* parse(SourceContext& context);

    private:
        Invoke* invoke;
        std::vector<Invoke::Value> results;
};


class Script
{
    public:
        Script() = default;

        void addModule(std::shared_ptr<Module>& module);
        void addAssertReturn(std::shared_ptr<AssertReturn>& assertReturn);
        void addInvoke(std::shared_ptr<Invoke>& invoke);

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

        std::vector<Command> commands;

};

};

#endif