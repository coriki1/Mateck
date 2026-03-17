#pragma once

#include <iostream>
#include <string>
#include <vector>


enum class TokenType {
    NUM,
    OP,
    VAR,
    FUNC,
    LPAREN,
    RPAREN,
////// IDENTIFIER: FUNCTION OR VARIABLE, UP TO FURTHER EVALUATION //////
    IDENTIFIER
};

class Token {
    TokenType type;
    std::string value;
public:
    Token(TokenType t, std::string v);

    std::string GetValue() const;
    TokenType GetType() const;


    void print() const;
};

class Lexer {
public:
    TokenType GetTokenType(char c) const;
    std::vector<Token> Tokenize(const std::string& e);
};
