#include "node.h"
#include <cmath>




namespace {

    double cot(double x)   { return 1.0 / std::tan(x); }
    double acot(double x)  { return std::atan(1.0 / x); }
    double coth(double x)  { return 1.0 / std::tanh(x); }
    double acoth(double x) { return std::atanh(1.0 / x); }
    
}

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

    std::unique_ptr<Node> argDifferentiated = arg->differentiate(var);
    return std::make_unique<UnaryOpNode>(op, std::move(argDifferentiated));

}

std::unique_ptr<Node> UnaryOpNode::clone() const {
    return std::make_unique<UnaryOpNode>(op, arg->clone());
}

bool UnaryOpNode::isConstant(const std::string& var) const {
    return (arg->isConstant(var));
}

/// FUNCNODE DEFINITIONS //////////////////////////////////////////////////////////////////////////////


double FuncNode::evaluate(const Environment& env) const {
    double val = arg->evaluate(env);

    switch (func) {

        // trigonometric functions
        case FunctionType::SIN : return std::sin(val);
        case FunctionType::COS : return std::cos(val);
        case FunctionType::TAN : return std::tan(val);
        case FunctionType::COT : return cot(val);

        case FunctionType::ARCSIN : return std::asin(val);
        case FunctionType::ARCCOS : return std::acos(val);
        case FunctionType::ARCTAN : return std::atan(val);
        case FunctionType::ARCCOT : return acot(val);

        // hyperbolic functions
        case FunctionType::SINH : return std::sinh(val);
        case FunctionType::COSH : return std::cosh(val);
        case FunctionType::TANH : return std::tanh(val);
        case FunctionType::COTH : return coth(val);

        case FunctionType::ASINH : return std::asinh(val);
        case FunctionType::ACOSH : return std::acosh(val);
        case FunctionType::ATANH : return std::atanh(val);
        case FunctionType::ACOTH : return acoth(val);
        
        // logarithmic functions
        case FunctionType::LN : return std::log(val);
        case FunctionType::LOG : return std::log10(val);
        
        // misc.
        case FunctionType::SQRT : return std::sqrt(val);
        case FunctionType::ABS : return std::abs(val);
        case FunctionType::FLOOR : return std::floor(val);
        case FunctionType::CEIL : return std::ceil(val);
        case FunctionType::ROUND : return std::round(val);
   
    }
    throw std::runtime_error("Unknown function type");
}


std::unique_ptr<Node> FuncNode::differentiate(const std::string& var) const {
    switch (func) {
        // trigonometric functions
        case FunctionType::SIN:    return differentiateSin(var);
        case FunctionType::COS:    return differentiateCos(var);
        case FunctionType::TAN:    return differentiateTan(var);
        case FunctionType::COT:    return differentiateCot(var);

        case FunctionType::ARCSIN: return differentiateArcsin(var);
        case FunctionType::ARCCOS: return differentiateArccos(var);
        case FunctionType::ARCTAN: return differentiateArctan(var);
        case FunctionType::ARCCOT: return differentiateArccot(var);

        // hyperbolic functions
        case FunctionType::SINH:   return differentiateSinh(var);
        case FunctionType::COSH:   return differentiateCosh(var);
        case FunctionType::TANH:   return differentiateTanh(var);
        case FunctionType::COTH:   return differentiateCoth(var);

        case FunctionType::ASINH:  return differentiateAsinh(var);
        case FunctionType::ACOSH:  return differentiateAcosh(var);
        case FunctionType::ATANH:  return differentiateAtanh(var);
        case FunctionType::ACOTH:  return differentiateAcoth(var);

        // logarithmic functions
        case FunctionType::LN:     return differentiateLn(var);
        case FunctionType::LOG:    return differentiateLog(var);

        // misc.
        case FunctionType::SQRT:   return differentiateSqrt(var);
        case FunctionType::ABS:    return differentiateAbs(var);

        case FunctionType::FLOOR:
        case FunctionType::CEIL:
        case FunctionType::ROUND:
            throw std::runtime_error("Cannot differentiate a discontinuous function (floor/ceil/round)");

        default:
            throw std::runtime_error("FuncNode::differentiate: function not implemented yet");
    }
}


// d/dx sin(f) = cos(f) * f'
std::unique_ptr<Node> FuncNode::differentiateSin(const std::string& var) const {
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL,
        std::make_unique<FuncNode>(FunctionType::COS, arg->clone()),
        arg->differentiate(var)
    );
}

// d/dx cos(f) = -sin(f) * f'
std::unique_ptr<Node> FuncNode::differentiateCos(const std::string& var) const {
    std::unique_ptr<Node> sinTerm = std::make_unique<FuncNode>(FunctionType::SIN, arg->clone());
    std::unique_ptr<Node> negSin = std::make_unique<UnaryOpNode>(UnaryOperator::SUB, std::move(sinTerm));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::move(negSin), arg->differentiate(var)
    );
}

// d/dx tan(f) = f' / cos(f)^2
std::unique_ptr<Node> FuncNode::differentiateTan(const std::string& var) const {
    std::unique_ptr<Node> cosTerm = std::make_unique<FuncNode>(FunctionType::COS, arg->clone());
    std::unique_ptr<Node> cosSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, std::move(cosTerm), std::make_unique<NumberNode>(2.0)
    );
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(cosSquared)
    );
}

// d/dx cot(f) = -f' / sin(f)^2
std::unique_ptr<Node> FuncNode::differentiateCot(const std::string& var) const {
    std::unique_ptr<Node> sinTerm = std::make_unique<FuncNode>(FunctionType::SIN, arg->clone());
    std::unique_ptr<Node> sinSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, std::move(sinTerm), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> negDerivative = std::make_unique<UnaryOpNode>(UnaryOperator::SUB, arg->differentiate(var));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, std::move(negDerivative), std::move(sinSquared)
    );
}

// d/dx arcsin(f) = f' / sqrt(1 - f^2)
std::unique_ptr<Node> FuncNode::differentiateArcsin(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> oneMinusFSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::SUB, std::make_unique<NumberNode>(1.0), std::move(fSquared)
    );
    std::unique_ptr<Node> sqrtTerm = std::make_unique<FuncNode>(FunctionType::SQRT, std::move(oneMinusFSquared));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(sqrtTerm)
    );
}

// d/dx arccos(f) = -f' / sqrt(1 - f^2)
std::unique_ptr<Node> FuncNode::differentiateArccos(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> oneMinusFSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::SUB, std::make_unique<NumberNode>(1.0), std::move(fSquared)
    );
    std::unique_ptr<Node> sqrtTerm = std::make_unique<FuncNode>(FunctionType::SQRT, std::move(oneMinusFSquared));
    std::unique_ptr<Node> negDerivative = std::make_unique<UnaryOpNode>(UnaryOperator::SUB, arg->differentiate(var));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, std::move(negDerivative), std::move(sqrtTerm)
    );
}

// d/dx arctan(f) = f' / (1 + f^2)
std::unique_ptr<Node> FuncNode::differentiateArctan(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> onePlusFSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::ADD, std::make_unique<NumberNode>(1.0), std::move(fSquared)
    );
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(onePlusFSquared)
    );
}

// d/dx arccot(f) = -f' / (1 + f^2)
std::unique_ptr<Node> FuncNode::differentiateArccot(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> onePlusFSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::ADD, std::make_unique<NumberNode>(1.0), std::move(fSquared)
    );
    std::unique_ptr<Node> negDerivative = std::make_unique<UnaryOpNode>(UnaryOperator::SUB, arg->differentiate(var));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, std::move(negDerivative), std::move(onePlusFSquared)
    );
}

// d/dx sinh(f) = cosh(f) * f'
std::unique_ptr<Node> FuncNode::differentiateSinh(const std::string& var) const {
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL,
        std::make_unique<FuncNode>(FunctionType::COSH, arg->clone()),
        arg->differentiate(var)
    );
}

// d/dx cosh(f) = sinh(f) * f'
std::unique_ptr<Node> FuncNode::differentiateCosh(const std::string& var) const {
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL,
        std::make_unique<FuncNode>(FunctionType::SINH, arg->clone()),
        arg->differentiate(var)
    );
}

// d/dx tanh(f) = f' / cosh(f)^2
std::unique_ptr<Node> FuncNode::differentiateTanh(const std::string& var) const {
    std::unique_ptr<Node> coshTerm = std::make_unique<FuncNode>(FunctionType::COSH, arg->clone());
    std::unique_ptr<Node> coshSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, std::move(coshTerm), std::make_unique<NumberNode>(2.0)
    );
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(coshSquared)
    );
}

// d/dx coth(f) = -f' / sinh(f)^2
std::unique_ptr<Node> FuncNode::differentiateCoth(const std::string& var) const {
    std::unique_ptr<Node> sinhTerm = std::make_unique<FuncNode>(FunctionType::SINH, arg->clone());
    std::unique_ptr<Node> sinhSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, std::move(sinhTerm), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> negDerivative = std::make_unique<UnaryOpNode>(UnaryOperator::SUB, arg->differentiate(var));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, std::move(negDerivative), std::move(sinhSquared)
    );
}

// d/dx asinh(f) = f' / sqrt(f^2 + 1)
std::unique_ptr<Node> FuncNode::differentiateAsinh(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> fSquaredPlusOne = std::make_unique<BinaryOpNode>(
        BinaryOperator::ADD, std::move(fSquared), std::make_unique<NumberNode>(1.0)
    );
    std::unique_ptr<Node> sqrtTerm = std::make_unique<FuncNode>(FunctionType::SQRT, std::move(fSquaredPlusOne));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(sqrtTerm)
    );
}

// d/dx acosh(f) = f' / sqrt(f^2 - 1)
std::unique_ptr<Node> FuncNode::differentiateAcosh(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> fSquaredMinusOne = std::make_unique<BinaryOpNode>(
        BinaryOperator::SUB, std::move(fSquared), std::make_unique<NumberNode>(1.0)
    );
    std::unique_ptr<Node> sqrtTerm = std::make_unique<FuncNode>(FunctionType::SQRT, std::move(fSquaredMinusOne));
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(sqrtTerm)
    );
}

// d/dx atanh(f) = f' / (1 - f^2)
std::unique_ptr<Node> FuncNode::differentiateAtanh(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> oneMinusFSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::SUB, std::make_unique<NumberNode>(1.0), std::move(fSquared)
    );
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(oneMinusFSquared)
    );
}

// d/dx acoth(f) = f' / (1 - f^2)
std::unique_ptr<Node> FuncNode::differentiateAcoth(const std::string& var) const {
    std::unique_ptr<Node> fSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::POW, arg->clone(), std::make_unique<NumberNode>(2.0)
    );
    std::unique_ptr<Node> oneMinusFSquared = std::make_unique<BinaryOpNode>(
        BinaryOperator::SUB, std::make_unique<NumberNode>(1.0), std::move(fSquared)
    );
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(oneMinusFSquared)
    );
}

// d/dx ln(f) = f' / f
std::unique_ptr<Node> FuncNode::differentiateLn(const std::string& var) const {
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), arg->clone()
    );
}

// d/dx log10(f) = f' / (f * ln(10))
std::unique_ptr<Node> FuncNode::differentiateLog(const std::string& var) const {
    std::unique_ptr<Node> ln10 = std::make_unique<NumberNode>(std::log(10.0));
    std::unique_ptr<Node> fTimesLn10 = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, arg->clone(), std::move(ln10)
    );
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(fTimesLn10)
    );
}

// d/dx sqrt(f) = f' / (2 * sqrt(f))
std::unique_ptr<Node> FuncNode::differentiateSqrt(const std::string& var) const {
    std::unique_ptr<Node> sqrtTerm = std::make_unique<FuncNode>(FunctionType::SQRT, arg->clone());
    std::unique_ptr<Node> twoTimesSqrt = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, std::make_unique<NumberNode>(2.0), std::move(sqrtTerm)
    );
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, arg->differentiate(var), std::move(twoTimesSqrt)
    );
}

// d/dx abs(f) = f' * f / abs(f)
std::unique_ptr<Node> FuncNode::differentiateAbs(const std::string& var) const {
    std::unique_ptr<Node> derivTimesArg = std::make_unique<BinaryOpNode>(
        BinaryOperator::MUL, arg->differentiate(var), arg->clone()
    );
    std::unique_ptr<Node> absTerm = std::make_unique<FuncNode>(FunctionType::ABS, arg->clone());
    return std::make_unique<BinaryOpNode>(
        BinaryOperator::DIV, std::move(derivTimesArg), std::move(absTerm)
    );
}

std::unique_ptr<Node> FuncNode::clone() const {
    return std::make_unique<FuncNode>(func, std::move(arg->clone()));
}

bool FuncNode::isConstant(const std::string& var) const {
    return (arg->isConstant(var));
}