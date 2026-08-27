#include "node.h"
#include <cmath>

/// NUMBERNODE DEFINITIONS //////////////////////////////////////////////////////////////

double NumberNode::evaluate(const Environment&) const {
    return value;
}

std::unique_ptr<Node> NumberNode::differentiate(const std::string&) const {
    return std::make_unique<NumberNode>(0.0);
}
std::unique_ptr<Node> NumberNode::clone() const {
    return std::make_unique<NumberNode>(*this);
}
bool NumberNode::isConstant(const std::string& ) const {
    return true;
}


