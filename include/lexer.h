#pragma once

#include "token.h"
#include "types.h"
#include <iostream>
#include <vector>
#include <unordered_set>

class Lexer {
    std::string input;
    size_t pos;
    char current;
public:

    Lexer(std::string in) : input(std::move(in)), pos(0), current(input.empty() ? '\0' : input[0]) {}
    
    void advance();
    char peek() const;
    void skipWhitespace();

    Token readNumber();
    Token readIdentifier();

    bool isOperator(char c) const { return c == '+' || c == '-' || c == '*' || c == '/' || c == '^'; }

    std::vector<Token> tokenize();
    
};