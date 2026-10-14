#pragma once
#include <optional>
#include <string>
#include <unordered_map>

using Environment = std::unordered_map<std::string, double>;

enum class TokenType {
    NUMBER,
    OPERATOR,
    VARIABLE,
    FUNCTION,
    CONSTANT,   /// pi, e
    LPAREN,
    RPAREN,
    END         /// always the last token, so the parser never runs off the list
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

/// NAME LOOKUPS (one table in types.cpp, used by the lexer, parser and toString) ///

std::string functionTypeToString(FunctionType f);
std::optional<FunctionType> stringToFunctionType(const std::string& name);

std::optional<double> constantValue(const std::string& name);