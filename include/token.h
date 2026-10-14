#pragma once

#include "types.h"
#include <iostream>
#include <utility>

class Token {
    TokenType type;
    std::string value;
    size_t position;    /// 0-based index of the token's first character in the input
public:
    Token(TokenType t, std::string val, size_t pos) : type(t), value(std::move(val)), position(pos) {}


    /// GETTERS ///
    TokenType getType() const { return type; }
    std::string getValue() const { return value; }
    size_t getPosition() const { return position; }
    void print() const;

};
