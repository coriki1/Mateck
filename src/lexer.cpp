#include "lexer.h"
#include "types.h"

static const std::unordered_set<std::string> FUNCTIONS = {
    "sin", "cos", "tan", "cot",
    "arcsin", "arccos", "arctan", "arccot",
    "sinh", "cosh", "tanh", "coth",
    "asinh", "acosh", "atanh", "acoth",
    "ln", "log", "sqrt", "abs",
    "floor", "ceil", "round"
};


std::vector<Token> Lexer::tokenize() { /// state machine thingy ///
    std::vector<Token> Tokens;

    while (current != '\0') {

        if (isdigit(current)) {
            Tokens.push_back(readNumber());
        }
        else if (isalpha(current)) {
            Tokens.push_back(readIdentifier());
        }
        else if (isOperator(current)) {
            Tokens.push_back(Token(TokenType::OPERATOR, std::string(1, current)));
            advance();
        }
        else if (current == '(') {
            Tokens.push_back(Token(TokenType::LPAREN, "("));
            advance();
        }
        else if (current == ')') {
            Tokens.push_back(Token(TokenType::RPAREN, ")"));
            advance();
        }
        else if (isspace(current)) {
            skipWhitespace();
        }
        else {
            throw std::runtime_error("Unsupported character: " + std::string(1, current));
        }

    }

    return Tokens;

}

void Lexer::advance() {

    if (pos < input.size()) {
        current = input[++pos];
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

Token Lexer::readIdentifier() { //updated so character chains are broken into individual variables

    std::string value = "";
    while (isalpha(current)) {
        value += current;
        advance();
    }

    if (FUNCTIONS.count(value)) {
        return Token(TokenType::FUNCTION, value);
    }


    if (value.size() > 1) {

        pos -= (value.size() - 1);
        current = input[pos];
        value = value.substr(0, 1);
    }

    return Token(TokenType::VARIABLE, value);
}