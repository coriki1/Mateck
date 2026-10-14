#include "lexer.h"
#include "types.h"
#include "errors.h"
#include <cctype>

// <cctype> functions are UB for negative chars (e.g. UTF-8 bytes), so cast to unsigned char first
static bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }
static bool isAlpha(char c) { return std::isalpha(static_cast<unsigned char>(c)); }
static bool isSpace(char c) { return std::isspace(static_cast<unsigned char>(c)); }


std::vector<Token> Lexer::tokenize() { /// state machine thingy ///
    std::vector<Token> Tokens;

    while (current != '\0') {

        if (isDigit(current) || (current == '.' && isDigit(peek()))) { // ".5" is a valid number
            Tokens.push_back(readNumber());
        }
        else if (isAlpha(current)) {
            Tokens.push_back(readIdentifier());
        }
        else if (isOperator(current)) {
            Tokens.push_back(Token(TokenType::OPERATOR, std::string(1, current), pos));
            advance();
        }
        else if (current == '(') {
            Tokens.push_back(Token(TokenType::LPAREN, "(", pos));
            advance();
        }
        else if (current == ')') {
            Tokens.push_back(Token(TokenType::RPAREN, ")", pos));
            advance();
        }
        else if (isSpace(current)) {
            skipWhitespace();
        }
        else {
            throw LexError("Unsupported character '" + std::string(1, current) + "'", pos);
        }

    }

    Tokens.push_back(Token(TokenType::END, "", input.size()));
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

    while (isSpace(current)) {
        advance();
    }
}

Token Lexer::readNumber() {

    size_t start = pos;
    std::string value = "";
    bool decimalPointFound = false;
    
    while (isDigit(current) || (current == '.' && !decimalPointFound)) {

        if (current == '.') {
            decimalPointFound = true;
        }

        value += current;
        advance();

    }

    if (current == '.') { // second decimal point, e.g. "1.2.3"
        throw LexError("Malformed number '" + value + ".'", start);
    }

    return Token(TokenType::NUMBER, value, start);
}

Token Lexer::readIdentifier() { // character chains are broken into individual variables, e.g. "xy" -> x, y

    size_t start = pos;
    size_t end = pos;
    while (end < input.size() && isAlpha(input[end])) {
        end++;
    }

    // longest function or constant name at the start of the chain wins,
    // e.g. "sinhx" -> sinh, x and "pix" -> pi, x
    for (size_t len = end - start; len >= 1; len--) {

        std::string candidate = input.substr(start, len);

        TokenType type;
        if (stringToFunctionType(candidate)) {
            type = TokenType::FUNCTION;
        }
        else if (constantValue(candidate)) {
            type = TokenType::CONSTANT;
        }
        else {
            continue;
        }

        for (size_t i = 0; i < len; i++) {
            advance();
        }
        return Token(type, candidate, start);
    }

    advance();
    return Token(TokenType::VARIABLE, std::string(1, input[start]), start);
}
