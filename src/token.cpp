#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <regex>
#include "token.h"


Token::Token(TokenType t, std::string v) : type(t), value(v) {}

TokenType Lexer::GetTokenType(char c) const {

    if(isdigit(c) || c == '.') { // NUM CHECK
        return TokenType::NUM;
    }
    else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') { //OP CHECK
        return TokenType::OP;
    }
    else if(isalpha(c)) { // IDENTIFIER CHECK
        return TokenType::IDENTIFIER;
    }
    else if(c == '(') { //LPAREN CHECK
        return TokenType::LPAREN;
    }
    else if(c == ')') { //RPAREN CHECK
        return TokenType::RPAREN;
    }
    else {
        throw "UNSUPPORTED CHARACTER";
    }

}

std::vector<Token> Lexer::Tokenize(const std::string& e) {
    std::string expression = std::regex_replace(e, std::regex(" "), "");
    std::vector<Token> TokenArr;
    TokenType CurrentType;
    std::string CurrentValue;
    size_t i = 0;

    while (i < expression.size()) {

        CurrentValue = "";
        CurrentType = GetTokenType(expression[i]);

       
        while (i < expression.size() && CurrentType == GetTokenType(expression[i])) {
            CurrentValue += expression[i];
            i++;
        }
        
        TokenArr.push_back(Token(CurrentType, CurrentValue));
    }
    return TokenArr;
}

std::string Token::GetValue() const {
    return value;
}
TokenType Token::GetType() const {
    return type;
}
//temp function, majd kesobb 
inline std::string TokenTypeToString(TokenType t) {
    switch(t) {
        case TokenType::NUM:        return "NUM";
        case TokenType::OP:         return "OP";
        case TokenType::VAR:        return "VAR";
        case TokenType::FUNC:       return "FUNC";
        case TokenType::LPAREN:     return "LPAREN";
        case TokenType::RPAREN:     return "RPAREN";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
    }
    return "UNKNOWN";
}

void Token::print() const {
    std::cout << value << " : " <<TokenTypeToString(type) << std::endl;
}