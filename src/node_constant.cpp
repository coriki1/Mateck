#include "node.h"

/// CONSTANTNODE DEFINITIONS ////////////////////////////////////////////////////////////

double ConstantNode::evaluate(const Environment&) const {
    return value;
}

std::unique_ptr<Node> ConstantNode::differentiate(const std::string&) const {
    return std::make_unique<NumberNode>(0.0);
}

std::unique_ptr<Node> ConstantNode::clone() const {
    return std::make_unique<ConstantNode>(*this);
}

bool ConstantNode::isConstant(const std::string&) const {
    return true;
}

std::unique_ptr<Node> ConstantNode::simplifyStep() const {
    return clone();
}

std::string ConstantNode::toString() const {
    return name;
}

bool ConstantNode::equals(const Node& other) const {
    auto* o = dynamic_cast<const ConstantNode*>(&other);
    return o && name == o->name;
}

int ConstantNode::precedence() const {
    return Precedence::ATOM;
}
