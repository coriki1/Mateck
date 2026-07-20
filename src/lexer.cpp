#include "lexer.h"
#include "types.h"

static const std::unordered_set<std::string> FUNCTIONS = {
    "sin", "cos", "tan", "cot",
    "arcsin", "arccos", "arctan", "arccot",
    "sinh", "cosh", "tanh", "coth",
    "asinh", "acosh", "atanh", "acoth",
    "ln", "log", "log2", "sqrt", "abs",
    "floor", "ceil", "round"
};


std::vector<Token> Lexer::tokenize() {

}

void Lexer::advance() {

    if (pos < input.size()) {
        current = input[pos++];
    } 
    else {
        current = '\0';
    }
}

char Lexer::peek() const {

    if (pos + 1 < input.size()) {
        return input[pos + 1];
    }
    else {
        return '\0';
    }
}

void Lexer::skipWhitespace() {

    if (isspace(current)) {
        advance();
    }
}

Token Lexer::readNumber() {

    std::string value = "";
    bool decimalPointFound = false;
    
    while (isdigit(current) || (current == '.' && !decimalPointFound)) {

        if (current == '.') {
            decimalPointFound = true;
        }

        value += current;
        advance();
    }
    
    return Token(TokenType::NUMBER, value);
}

Token Lexer::readIdentifier() {
    std::string value = "";

    while (isalpha(current)) {
        value += current;
        advance();
    }
    
    if (FUNCTIONS.count(value)) {
        return Token(TokenType::FUNCTION, value);
    }
    else {
        return Token(TokenType::VARIABLE, value);
    }

}