// ErrorHandler.h

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "Token.h"
#include "TreeNode.h"

#include <iostream>
#include <string>
#include <string_view>

namespace libwasm
{
class ErrorHandler
{
    public:
        ErrorHandler() = default;
        ErrorHandler(std::ostream& es)
          : errorStream(es)
        {
        }

        auto getErrorCount() const
        {
            return errorCount;
        }

        auto getWarningCount() const
        {
            return warningCount;
        }

        auto& getErrorStream()
        {
            return errorStream;
        }

    protected:
        unsigned errorCount = 0;
        unsigned warningCount = 0;
        std::ostream& errorStream = std::cerr;
};

class BinaryErrorHandler : public ErrorHandler
{
    public:
        BinaryErrorHandler() = default;
        BinaryErrorHandler(std::ostream& es)
           : ErrorHandler(es)
        {
        }

        template<typename... Ts>
        void error(const Ts&... ts)
        {
            showHeader("Error");

            ((errorStream << ts), ...);
            errorStream << std::endl;
            errorCount++;
        }

        template<typename... Ts>
        void errorWhen(bool condition, const Ts&... ts)
        {
            if (condition) {
                error(ts...);
            }
        }

        template<typename... Ts>
        void warning(const Ts&... ts)
        {
            showHeader("Warning");

            ((errorStream << ts), ...);
            errorStream << std::endl;
            warningCount++;
        }

        template<typename... Ts>
        void warningWhen(bool condition, const Ts&... ts)
        {
            if (condition) {
                warning(ts...);
            }
        }

        void setSectionName(std::string_view name)
        {
            sectionName = name;
        }

        void setEntryNumber(unsigned number)
        {
            entryNumber = number;
        }

        void resetInfo()
        {
            sectionName.clear();
            entryNumber = 0;
        }

    private:
        void showHeader(std::string_view type)
        {
            errorStream << type << ' ';
            if (!sectionName.empty()) {
                errorStream << "in " << sectionName << " section at entry " << entryNumber;
            }

            errorStream << ":\n    ";
        }

        std::string sectionName;
        unsigned entryNumber;
};

class SourceErrorHandler : public ErrorHandler
{
    public:
        SourceErrorHandler() = default;
        SourceErrorHandler(std::ostream& es)
            : ErrorHandler(es)
        {
        }

        template<typename... Ts>
        void error(size_t lineNumber, size_t columnNumber, const Ts&... ts)
        {
            showHeader("Error", lineNumber, columnNumber);

            ((errorStream << ts), ...);
            errorStream << std::endl;
            errorCount++;
        }

        template<typename... Ts>
        void error(const Token& token, const Ts&... ts)
        {
            error(token.getLineNumber(), token.getColumnNumber(), ts...);
        }

        template<typename... Ts>
        void errorWhen(bool condition, size_t lineNumber, size_t columnNumber, const Ts&... ts)
        {
            if (condition) {
                error(lineNumber, columnNumber, ts...);
            }
        }

        template<typename... Ts>
        void errorWhen(bool condition, const Token& token, const Ts&... ts)
        {
            if (condition) {
                error(token, ts...);
            }
        }

        template<typename... Ts>
        void warning(size_t lineNumber, size_t columnNumber, const Ts&... ts)
        {
            showHeader("Warning", lineNumber, columnNumber);

            ((errorStream << ts), ...);
            errorStream << std::endl;
            warningCount++;
        }

        template<typename... Ts>
        void warningWhen(bool condition, size_t lineNumber, size_t columnNumber, const Ts&... ts)
        {
            if (condition) {
                warning(lineNumber, columnNumber, ts...);
            }
        }

        template<typename... Ts>
        void warningWhen(bool condition, const Token& token, const Ts&... ts)
        {
            if (condition) {
                warning(token, ts...);
            }
        }

        template<typename... Ts>
        void expected(const Token& token, const Ts&... ts)
        {
            error(token, "found '", token.getValue(), "', expected ", ts..., '.');
        }

    private:
        void showHeader(std::string_view type, size_t lineNumber, size_t columnNumber)
        {
            errorStream << type << " at line " << lineNumber << '(' << columnNumber << "):\n    ";
        }
};

class CheckErrorHandler : public ErrorHandler
{
    public:
        CheckErrorHandler() = default;
        CheckErrorHandler(std::ostream& es)
          : ErrorHandler(es)
        {
        }

        template<typename... Ts>
        void error(size_t lineNumber, size_t columnNumber, const Ts&... ts)
        {
            showHeader("Error", lineNumber, columnNumber);

            ((errorStream << ts), ...);
            errorStream << std::endl;
            errorCount++;
        }

        template<typename... Ts>
        void error(const TreeNode* node, const Ts&... ts)
        {
            error(node->getLineNumber(), node->getColumnNumber(), ts...);
        }

        template<typename... Ts>
        void errorWhen(bool condition, const TreeNode* node, const Ts&... ts)
        {
            if (condition) {
                error(node, ts...);
            }
        }

        template<typename... Ts>
        void warning(size_t lineNumber, size_t columnNumber, const Ts&... ts)
        {
            showHeader("Warning", lineNumber, columnNumber);

            ((errorStream << ts), ...);
            errorStream << std::endl;
            warningCount++;
        }

        template<typename... Ts>
        void warning(const TreeNode* node, const Ts&... ts)
        {
            warning(node->getLineNumber(), node->getColumnNumber(), ts...);
        }

        template<typename... Ts>
        void warningWhen(bool condition, const TreeNode* node, const Ts&... ts)
        {
            if (condition) {
                warning(node, ts...);
            }
        }

    private:
        void showHeader(std::string_view type, size_t lineNumber, size_t columnNumber)
        {
            if (lineNumber == 0) {
                errorStream << type << " at position 0x" << std::hex << columnNumber << std::dec << ":\n    ";
            } else {
                errorStream << type << " at line " << lineNumber << '(' << columnNumber << "):\n    ";
            }
        }
};
};

#endif
