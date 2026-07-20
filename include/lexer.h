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

    bool isOperator(char c) const { return c == '+' || c == '-' || c == '*' || c == '/' || c == '^'; }

    std::vector<Token> tokenize();
    
};