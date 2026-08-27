#include <cmath>
#include "node.h"

/// VARNODE DEFINITIONS /////////////////////////////////////////////////////////////////

double VarNode::evaluate(const Environment& env) const {
    auto it = env.find(name);
    if (it == env.end()) {
        throw std::runtime_error("Undefined variable: " + name);
    }
    return it->second;
}

std::unique_ptr<Node> VarNode::differentiate(const std::string& var) const {
    if (name == var) {
        return std::make_unique<NumberNode>(1.0);
    }
    else {
        return std::make_unique<NumberNode>(0.0);
    }
}

std::unique_ptr<Node> VarNode::clone() const {
    return std::make_unique<VarNode>(*this);
}

bool VarNode::isConstant(const std::string& var) const {
    return name != var;
}
