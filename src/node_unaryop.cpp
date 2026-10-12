#include <cmath>
#include <stdexcept>
#include "node.h"

/// UNARYOPNODE DEFINITIONS //////////////////////////////////////////////////////////////////////////////

double UnaryOpNode::evaluate(const Environment& env) const {

    double val = arg->evaluate(env);
    
    switch (op) {
        case UnaryOperator::ADD: return val;
        case UnaryOperator::SUB: return -val;
    }
    
    throw std::runtime_error("Unknown unary operator");
}

std::unique_ptr<Node> UnaryOpNode::differentiate(const std::string& var) const {

    if (isConstant(var)) { // avoids 0 * inf = NaN when the subtree does not depend on var
        return std::make_unique<NumberNode>(0.0);
    }

    std::unique_ptr<Node> argDifferentiated = arg->differentiate(var);
    return std::make_unique<UnaryOpNode>(op, std::move(argDifferentiated));

}

std::unique_ptr<Node> UnaryOpNode::clone() const {
    return std::make_unique<UnaryOpNode>(op, arg->clone());
}

bool UnaryOpNode::isConstant(const std::string& var) const {
    return (arg->isConstant(var));
}

