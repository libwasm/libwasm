// Context.h

#ifndef CONTEXT_H
#define CONTEXT_H

#include "ErrorHandler.h"
#include "common.h"
#include "DataBuffer.h"
#include "TokenBuffer.h"
#include "Encodings.h"

#include <memory>
#include <vector>

namespace libwasm
{
class Module;

class Context
{
    public:
        Context();
        Context(const Context& other);

        auto* getModule()
        {
            return module;
        }

        auto& getModules()
        {
            return modules;
        }

        DataBuffer& data()
        {
            return dataBuffer;
        }

    protected:
        DataBuffer dataBuffer;
        Module* module;
        std::vector<std::shared_ptr<Module>> modules;
};

class BinaryContext : public Context
{
    public:
        BinaryContext(BinaryErrorHandler& error)
          : errorHandler(error)
        {
        }

        BinaryContext(Context& other, BinaryErrorHandler& error)
          : Context(other), errorHandler(error)
        {
        }

        BinaryErrorHandler& msgs()
        {
            return errorHandler;
        }

        void dump(std::ostream& os);
        void write(std::ostream& os);

        template<typename T, typename... Ts>
        T* makeTreeNode(const Ts&... ts)
        {
            T* result = new T(ts...);

            result->setColumnNumber(dataBuffer.getPos());
            return result;
        }

    private:
        BinaryErrorHandler& errorHandler;

        void dumpSections(std::ostream& os);
        void writeHeader();
        void writeSections();
        void writeFile(std::ostream& os);
};

class SourceContext : public Context
{
    public:
        SourceContext(TokenBuffer& data, SourceErrorHandler& error)
          : tokenBuffer(data), errorHandler(error)
        {
        }

        auto& tokens()
        {
            return tokenBuffer;
        }

        auto& msgs()
        {
            return errorHandler;
        }

        void write(std::ostream& os);

        template<typename T, typename... Ts>
        T* makeTreeNode(const Ts&... ts)
        {
            T* result = new T(ts...);

            result->setLineNumber(tokenBuffer.peekToken(-1).getLineNumber());
            result->setColumnNumber(tokenBuffer.peekToken(-1).getColumnNumber());
            return result;
        }

    private:
        TokenBuffer& tokenBuffer;
        SourceErrorHandler& errorHandler;
};

class CheckContext : public Context
{
    public:
        CheckContext(CheckErrorHandler& error)
          : errorHandler(error)
        {
        }

        CheckContext(Context& other, CheckErrorHandler& error)
          : Context(other), errorHandler(error)
        {
        }

        auto& msgs()
        {
            return errorHandler;
        }

        bool checkSemantics();
        void checkDataCount(TreeNode* node, uint32_t count);
        void checkElementType(TreeNode* node, const ValueType& type);
        void checkEventIndex(TreeNode* node, uint32_t index);
        void checkEventType(TreeNode* node, const EventType& type);
        void checkExternalType(TreeNode* node, const ExternalType& type);
        void checkFunctionIndex(TreeNode* node, uint32_t index);
        void checkGlobalIndex(TreeNode* node, uint32_t index);
        void checkLimits(TreeNode* node, const Limits& limits);
        void checkMemoryIndex(TreeNode* node, uint32_t index);
        void checkMut(TreeNode* node, Mut& mut);
        void checkTableIndex(TreeNode* node, uint32_t index);
        void checkTypeIndex(TreeNode* node, uint32_t count);
        void checkValueType(TreeNode* node, const ValueType& type);

    private:
        CheckErrorHandler& errorHandler;
};
};

#endif
