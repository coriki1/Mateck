#pragma once

#include "types.h"
#include <iostream>

class Token {
    TokenType type;
    std::string value;
public:
    Token(TokenType t, std::string val) : type(t), value(val) {}


    /// GETTERS ///
    TokenType getType() const { return type; }
    std::string getValue() const { return value; }

};
