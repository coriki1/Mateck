#include <stdexcept>

#include "types.h"

namespace {

    struct FunctionName {
        FunctionType type;
        const char* name;
    };

    // the only list of function names: add a new function here and in the FunctionType enum
    const FunctionName FUNCTION_NAMES[] = {
        {FunctionType::SIN,    "sin"},    {FunctionType::COS,    "cos"},
        {FunctionType::TAN,    "tan"},    {FunctionType::COT,    "cot"},
        {FunctionType::ARCSIN, "arcsin"}, {FunctionType::ARCCOS, "arccos"},
        {FunctionType::ARCTAN, "arctan"}, {FunctionType::ARCCOT, "arccot"},
        {FunctionType::SINH,   "sinh"},   {FunctionType::COSH,   "cosh"},
        {FunctionType::TANH,   "tanh"},   {FunctionType::COTH,   "coth"},
        {FunctionType::ASINH,  "asinh"},  {FunctionType::ACOSH,  "acosh"},
        {FunctionType::ATANH,  "atanh"},  {FunctionType::ACOTH,  "acoth"},
        {FunctionType::LN,     "ln"},     {FunctionType::LOG,    "log"},
        {FunctionType::SQRT,   "sqrt"},   {FunctionType::ABS,    "abs"},
        {FunctionType::FLOOR,  "floor"},  {FunctionType::CEIL,   "ceil"},
        {FunctionType::ROUND,  "round"},
    };

    struct ConstantName {
        const char* name;
        double value;
    };

    const ConstantName CONSTANTS[] = {
        {"pi", 3.14159265358979323846},
        {"e",  2.71828182845904523536},
    };

}

std::string tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:   return "NUMBER";
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::VARIABLE: return "VARIABLE";
        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::CONSTANT: return "CONSTANT";
        case TokenType::LPAREN:   return "LPAREN";
        case TokenType::RPAREN:   return "RPAREN";
        case TokenType::END:      return "END";
        default:                  return "UNKNOWN";
    }
}

std::string functionTypeToString(FunctionType f) {
    for (const FunctionName& entry : FUNCTION_NAMES) {
        if (entry.type == f) {
            return entry.name;
        }
    }
    throw std::logic_error("functionTypeToString: function missing from FUNCTION_NAMES");
}

std::optional<FunctionType> stringToFunctionType(const std::string& name) {
    for (const FunctionName& entry : FUNCTION_NAMES) {
        if (name == entry.name) {
            return entry.type;
        }
    }
    return std::nullopt;
}

std::optional<double> constantValue(const std::string& name) {
    for (const ConstantName& entry : CONSTANTS) {
        if (name == entry.name) {
            return entry.value;
        }
    }
    return std::nullopt;
}
