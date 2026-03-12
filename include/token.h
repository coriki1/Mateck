#include <string>
#include <vector>

#ifndef TOKEN_H
#define TOKEN_H

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
    std::string value;
    TokenType type;
public:
    Token(std::string v, TokenType t);
};

class Lexer {
public:
    TokenType GetTokenType(char c) const;
    std::vector<Token> Tokenize(std::string expression);
};

#endif