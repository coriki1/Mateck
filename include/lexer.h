#pragma once

#include "token.h"
#include "types.h"
#include <iostream>
#include <vector>
#include <unordered_set>

class Lexer {
    const std::string& input;
    size_t pos;
    char current;
public:

     
    Lexer(const std::string& in) : input(in), pos(0), current(in.empty() ? '\0' : in[0]) {} 
    
    void advance();
    char peek() const;
    void skipWhitespace();

    Token readNumber();
    Token readIdentifier();

    std::vector<Token> tokenize();
    
private:
    static const std::unordered_set<std::string> FUNCTIONS = {
    "sin", "cos", "tan", "cot",
    "arcsin", "arccos", "arctan", "arccot",
    "sinh", "cosh", "tanh", "coth",
    "asinh", "acosh", "atanh", "acoth",
    "ln", "log", "log2", "sqrt", "abs",
    "floor", "ceil", "round"
};
};