#include "token.h"

void Token::print() const {
    std::cout << "[" << tokenTypeToString(type) << ": " << value << "]\n";
}