// TreeNode.h

#ifndef TREENODE_H
#define TREENODE_H

#include <cstdlib>
namespace libwasm
{

class TreeNode
{
    public:
        TreeNode() = default;
        virtual ~TreeNode() = default;

        auto getLineNumber() const
        {
            return lineNumber;
        }

        void setLineNumber(size_t n)
        {
            lineNumber = n;
        }

        auto getColumnNumber() const
        {
            return columnNumber;
        }

        void setColumnNumber(size_t n)
        {
            columnNumber = n;
        }

    protected:
        size_t lineNumber = 0;
        size_t columnNumber = 0;
};
};

#endif
