#include <vector>
#include <string>
#include <cctype>
#include "token.h"


Token::Token(std::string v, TokenType t) : value(v), type(t) {}

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

std::vector<Token> Lexer::Tokenize(std::string expression) {

    std::vector<Token> TokenArr;
    TokenType CurrentType;
    std::string CurrentValue;
    int counter = 0;

    for (int i = 0; i < expression.length(); i++) {

        counter = 0;
        CurrentValue = "";
        CurrentType == GetTokenType(expression[i]);
       
        for (int j = i; GetTokenType(expression[j]) != CurrentType; j++) {
            CurrentValue += expression[j];
            counter++;
        }
        
        TokenArr.push_back(Token(CurrentValue, CurrentType));
        i += counter;
    }
}