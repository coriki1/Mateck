#include <cmath>
#include "node.h"

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
    
    throw std::runtime_error("Unknown binary operator");

}

std::unique_ptr<Node> BinaryOpNode::differentiate(const std::string& var) const {

    switch (op) {
        case BinaryOperator::ADD: 
        case BinaryOperator::SUB: return differentiateLinear(var);
        case BinaryOperator::MUL: return differentiateProduct(var);
        case BinaryOperator::DIV: return differentiateQuotient(var);
        case BinaryOperator::POW: return differentiatePower(var);
    }

    throw std::runtime_error("Unknown binary operator");
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

