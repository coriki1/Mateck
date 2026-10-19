#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "node.h"
#include "errors.h"

/// VARNODE DEFINITIONS /////////////////////////////////////////////////////////////////

double VarNode::evaluate(const Environment& env) const {
    auto it = env.find(name);
    if (it == env.end()) {
        throw EvalError("Undefined variable: " + name);
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

std::unique_ptr<Node> VarNode::simplifyStep() const {
    return clone();
}

std::string VarNode::toString() const {
    return name;
}

int VarNode::precedence() const {
    return Precedence::ATOM;
}

bool VarNode::equals(const Node& other) const {
    auto* o = dynamic_cast<const VarNode*>(&other);
    return o && name == o->name;
}

std::unique_ptr<Node> VarNode::substitute(const std::string& var, const Node& value) const {
    if (name == var) {
        return value.clone();
    }
    return clone();
}

void VarNode::collectVariables(std::vector<std::string>& out) const {
    if (std::find(out.begin(), out.end(), name) == out.end()) {
        out.push_back(name);
    }
}
