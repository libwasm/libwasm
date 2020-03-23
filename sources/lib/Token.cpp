// Token.cpp

#include "Token.h"

#include <iostream>

void Token::dump(std::ostream& os) const
{
    os << lineNumber << " (" << columnNumber << ") ";
    switch (kind) {
        default: os << "none"; break;
        case keyword: os << "keyword"; break;
        case integer: os << "integer"; break;
        case floating: os << "floating"; break;
        case string: os << "string"; break;
        case id: os << "id"; break;
        case parenthesis: os << "parenthesis"; break;
    }

    os << " '" << value << "'\n";
}

