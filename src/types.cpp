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

std::string functionTypeToString(FunctionType f) {
    switch (f) {
        case FunctionType::SIN:    return "sin";
        case FunctionType::COS:    return "cos";
        case FunctionType::TAN:    return "tan";
        case FunctionType::COT:    return "cot";
        case FunctionType::ARCSIN: return "arcsin";
        case FunctionType::ARCCOS: return "arccos";
        case FunctionType::ARCTAN: return "arctan";
        case FunctionType::ARCCOT: return "arccot";
        case FunctionType::SINH:   return "sinh";
        case FunctionType::COSH:   return "cosh";
        case FunctionType::TANH:   return "tanh";
        case FunctionType::COTH:   return "coth";
        case FunctionType::ASINH:  return "asinh";
        case FunctionType::ACOSH:  return "acosh";
        case FunctionType::ATANH:  return "atanh";
        case FunctionType::ACOTH:  return "acoth";
        case FunctionType::LN:     return "ln";
        case FunctionType::LOG:    return "log";
        case FunctionType::SQRT:   return "sqrt";
        case FunctionType::ABS:    return "abs";
        case FunctionType::FLOOR:  return "floor";
        case FunctionType::CEIL:   return "ceil";
        case FunctionType::ROUND:  return "round";
        default:                   return "unknown";
    }
}
