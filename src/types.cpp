#include "token.h"
#include "types.h"

std::string tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:   return "NUMBER";
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::VARIABLE: return "VARIABLE";
        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::LPAREN:   return "LPAREN";
        case TokenType::RPAREN:   return "RPAREN";
        default:                  return "UNKNOWN";
    }
}