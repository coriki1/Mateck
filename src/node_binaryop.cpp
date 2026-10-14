#include <cmath>
#include <stdexcept>
#include "node.h"
#include "simplify.h"

/// BINARYOPNODE DEFINITIONS ////////////////////////////////////////////////////////////


double BinaryOpNode::evaluate(const Environment& env) const {

    double l = left->evaluate(env);
    double r = right->evaluate(env);
    
    switch (op) {
        case BinaryOperator::ADD: return l + r;
        case BinaryOperator::SUB: return l - r;
        case BinaryOperator::MUL: return l * r;
        case BinaryOperator::DIV: return l / r;
        case BinaryOperator::POW: return std::pow(l, r);
    }
    
    throw std::logic_error("Unknown binary operator");

}

std::unique_ptr<Node> BinaryOpNode::differentiate(const std::string& var) const {

    if (isConstant(var)) { // avoids 0 * inf = NaN when the subtree does not depend on var
        return std::make_unique<NumberNode>(0.0);
    }

    switch (op) {
        case BinaryOperator::ADD: 
        case BinaryOperator::SUB: return differentiateLinear(var);
        case BinaryOperator::MUL: return differentiateProduct(var);
        case BinaryOperator::DIV: return differentiateQuotient(var);
        case BinaryOperator::POW: return differentiatePower(var);
    }

    throw std::logic_error("Unknown binary operator");
}

std::unique_ptr<Node> BinaryOpNode::differentiateLinear(const std::string& var) const {

     return std::make_unique<BinaryOpNode>(op, left->differentiate(var), right->differentiate(var));
}

std::unique_ptr<Node> BinaryOpNode::differentiateProduct(const std::string& var) const {

    // f'*g + f*g'

    std::unique_ptr<Node> leftDifferentiated = left->differentiate(var); 
    std::unique_ptr<Node> rightDifferentiated = right->differentiate(var); 

    std::unique_ptr<Node> newLeft = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::move(leftDifferentiated), right->clone());

    std::unique_ptr<Node> newRight = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, left->clone(), std::move(rightDifferentiated));


    return std::make_unique<BinaryOpNode>(BinaryOperator::ADD, std::move(newLeft), std::move(newRight));
}


std::unique_ptr<Node> BinaryOpNode::differentiateQuotient(const std::string& var) const {

    // (f'*g - f*g') / g^2

    std::unique_ptr<Node> numerator = std::make_unique<BinaryOpNode>(
        BinaryOperator::SUB,
        std::make_unique<BinaryOpNode>(BinaryOperator::MUL, left->differentiate(var), right->clone()),
        std::make_unique<BinaryOpNode>(BinaryOperator::MUL, left->clone(), right->differentiate(var))
    );

    std::unique_ptr<Node> denominator = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, right->clone(), right->clone()
    );

    return std::make_unique<BinaryOpNode>(BinaryOperator::DIV, std::move(numerator), std::move(denominator));
}

std::unique_ptr<Node> BinaryOpNode::differentiatePower(const std::string& var) const {

    if (right->isConstant(var)) { // exponent is constant -> apply the power rule
        return differentiatePowerConstExponent(var);
    }
    else if (left->isConstant(var)) { // base is constant, exponent is not -> apply the exponential rule
        return differentiatePowerConstBase(var);
    }
    else { // both base and exponent depend on the variable -> apply logarithmic differentiation
        return differentiatePowerGeneral(var);
    }

}

std::unique_ptr<Node> BinaryOpNode::differentiatePowerConstExponent(const std::string& var) const {

    // power rule:  n * f^(n-1) * f'

    std::unique_ptr<Node> coefficient = right->clone();

    std::unique_ptr<Node> exponentMinusOne = std::make_unique<BinaryOpNode>(
        BinaryOperator::SUB, right->clone(), std::make_unique<NumberNode>(1.0)
    );

    std::unique_ptr<Node> powerTerm = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, left->clone(), std::move(exponentMinusOne)
    );

    std::unique_ptr<Node> coefTimesPower = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::move(coefficient), std::move(powerTerm)
    );

    return std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::move(coefTimesPower), left->differentiate(var)
    );

}

std::unique_ptr<Node> BinaryOpNode::differentiatePowerConstBase(const std::string& var) const {

    // f^g * ln(f) * g'

    std::unique_ptr<Node> powerTerm = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, left->clone(), right->clone()
    );

    std::unique_ptr<Node> lnTerm = std::make_unique<FuncNode>(
        FunctionType::LN, left->clone()
    );

    std::unique_ptr<Node> powerTimesLn = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::move(powerTerm), std::move(lnTerm)
    );

    return std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::move(powerTimesLn), right->differentiate(var)
    );

}

std::unique_ptr<Node> BinaryOpNode::differentiatePowerGeneral(const std::string& var) const {

    // f^g * (g'*ln(f) + g*f'/f)

    std::unique_ptr<Node> powerTerm = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, left->clone(), right->clone()
    );

    std::unique_ptr<Node> term1 = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL,
        right->differentiate(var),
        std::make_unique<FuncNode>(FunctionType::LN, left->clone())
    );

    std::unique_ptr<Node> term2 = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL,
        right->clone(),
        std::make_unique<BinaryOpNode>(BinaryOperator::DIV, left->differentiate(var), left->clone())
    );

    std::unique_ptr<Node> sumTerm = std::make_unique<BinaryOpNode>(
        BinaryOperator::ADD, std::move(term1), std::move(term2)
    );

    return std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::move(powerTerm), std::move(sumTerm)
    );
}


std::unique_ptr<Node> BinaryOpNode::clone() const {
    return std::make_unique<BinaryOpNode>(op, left->clone(), right->clone());
}

bool BinaryOpNode::isConstant(const std::string& var) const {
    return (left->isConstant(var) && right->isConstant(var));
}

std::unique_ptr<Node> BinaryOpNode::simplifyStep() const {

    std::unique_ptr<Node> l = left->simplifyStep();
    std::unique_ptr<Node> r = right->simplifyStep();

    std::optional<double> lv = l->numericValue();
    std::optional<double> rv = r->numericValue();

    if (lv && rv) { // 2 * 3 -> 6, but 1 / 0 stays as it is
        double v = BinaryOpNode(op, l->clone(), r->clone()).evaluate(Environment{});
        if (std::isfinite(v)) {
            return std::make_unique<NumberNode>(v);
        }
    }

    auto simplified = std::make_unique<BinaryOpNode>(op, std::move(l), std::move(r));

    switch (op) {
        case BinaryOperator::ADD:
        case BinaryOperator::SUB: return simplifySum(*simplified);       // x + 1 - x -> 1
        case BinaryOperator::MUL:
        case BinaryOperator::DIV: return simplifyProduct(*simplified);   // 2 * (3 * x) -> 6 * x

        case BinaryOperator::POW:
            if (rv == 0.0) return std::make_unique<NumberNode>(1.0);   // f^0 -> 1
            if (rv == 1.0) return std::move(simplified->left);            // f^1 -> f
            if (lv == 1.0) return std::make_unique<NumberNode>(1.0);   // 1^f -> 1
            break;
    }

    return simplified;
}

std::string BinaryOpNode::toString() const {

    int p = precedence();

    // ^ is right-associative: (a^b)^c needs parentheses on the left,
    // - and / are not associative: a - (b + c) and a / (b * c) need them on the right
    bool leftParens  = left->precedence() < p
                    || (op == BinaryOperator::POW && left->precedence() == p);
    bool rightParens = right->precedence() < p
                    || (right->precedence() == p && (op == BinaryOperator::SUB || op == BinaryOperator::DIV));

    std::string symbol;
    switch (op) {
        case BinaryOperator::ADD: symbol = " + "; break;
        case BinaryOperator::SUB: symbol = " - "; break;
        case BinaryOperator::MUL: symbol = " * "; break;
        case BinaryOperator::DIV: symbol = " / "; break;
        case BinaryOperator::POW: symbol = "^";   break;
    }

    std::string l = left->toString();
    std::string r = right->toString();

    return (leftParens ? "(" + l + ")" : l) + symbol + (rightParens ? "(" + r + ")" : r);
}

int BinaryOpNode::precedence() const {
    switch (op) {
        case BinaryOperator::ADD:
        case BinaryOperator::SUB: return Precedence::ADDITIVE;
        case BinaryOperator::MUL:
        case BinaryOperator::DIV: return Precedence::MULTIPLICATIVE;
        case BinaryOperator::POW: return Precedence::POWER;
    }
    throw std::logic_error("Unknown binary operator");
}

bool BinaryOpNode::equals(const Node& other) const {
    auto* o = dynamic_cast<const BinaryOpNode*>(&other);
    return o && op == o->op && *left == *o->left && *right == *o->right;
}
