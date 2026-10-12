#include <cmath>
#include <stdexcept>
#include "node.h"
#include "simplify.h"

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

std::unique_ptr<Node> UnaryOpNode::simplifyStep() const {

    std::unique_ptr<Node> a = arg->simplifyStep();

    if (op == UnaryOperator::ADD) { // +f -> f
        return a;
    }

    if (auto v = a->numericValue()) { // -(3) -> -3
        return std::make_unique<NumberNode>(-*v);
    }

    // -(-f) -> f, -(x + 1) -> -x - 1, -(2 * x) -> -2 * x
    return simplifySum(UnaryOpNode(op, std::move(a)));
}

std::string UnaryOpNode::toString() const {

    std::string sign = (op == UnaryOperator::SUB) ? "-" : "+";

    // "--x" would be confusing, so a nested unary operator also gets parentheses
    if (arg->precedence() <= Precedence::UNARY) {
        return sign + "(" + arg->toString() + ")";
    }
    return sign + arg->toString();
}

int UnaryOpNode::precedence() const {
    return Precedence::UNARY;
}

bool UnaryOpNode::equals(const Node& other) const {
    auto* o = dynamic_cast<const UnaryOpNode*>(&other);
    return o && op == o->op && *arg == *o->arg;
}
