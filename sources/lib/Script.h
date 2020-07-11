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
        ScriptValue()
        {
        }

        void generateC(std::ostream& os) const;
        void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber) const;
        static ScriptValue parse(SourceContext& context);

        enum Nan : uint8_t
        {
            none,
            canonical,
            arithmetic
        };

        struct I8
        {
            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane = -1) const;
            void generateC(std::ostream& os) const;
            static I8 parse(SourceContext& context);

            uint8_t value = 0;
            std::string_view string;
        };

        struct I16
        {
            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane = -1) const;
            void generateC(std::ostream& os) const;
            static I16 parse(SourceContext& context);

            uint16_t value = 0;
            std::string_view string;
        };

        struct I32
        {
            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane = -1) const;
            void generateC(std::ostream& os) const;
            static I32 parse(SourceContext& context);

            uint32_t value = 0;
            std::string_view string;
        };

        struct I64
        {
            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane = -1) const;
            void generateC(std::ostream& os) const;
            static I64 parse(SourceContext& context);

            uint64_t value = 0;
            std::string_view string;
        };

        struct F32
        {
            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane = -1) const;
            void generateC(std::ostream& os) const;
            static F32 parse(SourceContext& context);

            Nan nan = Nan::none;
            float value = 0;
            std::string_view string;
        };

        struct F64
        {
            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane = -1) const;
            void generateC(std::ostream& os) const;
            static F64 parse(SourceContext& context);

            Nan nan = Nan::none;
            double value = 0;
            std::string_view string;
        };

        struct V128
        {
            V128()
            {
            }

            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber) const;
            void generateC(std::ostream& os) const;
            static V128 parse(SourceContext& context);

            union {
                I8  i8x16[16];
                I16 i16x8[8];
                I32 i32x4[4];
                I64 i64x2[2];
                F32 f32x4[4];
                F64 f64x2[2];
            };

            ValueType type = ValueType::void_;
            unsigned count = 0;
        };

        struct ERef
        {
            void generateAssert(std::ostream& os, size_t lineNumber, unsigned resultNumber, int lane = -1) const;
            void generateC(std::ostream& os) const;
            static ERef parse(SourceContext& context);

            bool isNull = false;
            uint64_t value = 0;
            std::string_view string;
        };

        union
        {
            I32  i32;
            I64  i64;
            F32  f32;
            F64  f64;
            V128 v128;
            ERef eref;
        };

        ValueType type = ValueType::void_;
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
