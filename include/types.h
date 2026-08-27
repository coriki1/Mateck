#pragma once
#include <string>
#include <unordered_map>

using Environment = std::unordered_map<std::string, double>;

enum class TokenType {
    NUMBER,
    OPERATOR,
    VARIABLE,
    FUNCTION,
    LPAREN,
    RPAREN
};

enum class BinaryOperator {
    ADD,
    SUB,
    MUL,
    DIV,
    POW
};

enum class UnaryOperator {
    ADD,
    SUB
};

enum class FunctionType {

    ////// TRIGONOMETRIC FUNCTIONS //////
    SIN, COS, TAN, COT,
    ARCSIN, ARCCOS, ARCTAN, ARCCOT,

    ////// HYPERBOLIC FUNCTIONS //////
    SINH, COSH, TANH, COTH,
    ASINH, ACOSH, ATANH, ACOTH,

    ////// MISC. //////

    LN,     /// BASE E LOG
    LOG,    /// BASE 10 LOG


    SQRT, ABS, FLOOR, CEIL, ROUND

};

std::string tokenTypeToString(TokenType t);