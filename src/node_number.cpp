#include "node.h"
#include "simplify.h"
#include "format.h"
#include <sstream>
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

std::unique_ptr<Node> NumberNode::simplifyStep() const {
    return buildNumber(value); // 0.5 -> 1/2, so that it matches 1/2 written as a fraction
}

std::string NumberNode::toString() const {
    if (value == 0.0) { // avoid printing "-0"
        return "0";
    }
    std::ostringstream out;
    out << value;
    return out.str();
}

int NumberNode::precedence() const {
    return value < 0 ? Precedence::UNARY : Precedence::ATOM; // -1^2 would read as -(1^2)
}

std::optional<double> NumberNode::numericValue() const {
    return value;
}

bool NumberNode::equals(const Node& other) const {
    std::optional<double> o = other.numericValue();
    return o && value == *o;
}

std::unique_ptr<Node> NumberNode::substitute(const std::string&, const Node&) const {
    return clone();
}

void NumberNode::collectVariables(std::vector<std::string>&) const {
}

std::vector<const Node*> NumberNode::children() const {
    return {};
}

std::string NumberNode::label() const {
    return formatNumber(value);
}
