#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "types.h"


/// PARENT CLASS : NODE /////////////////////////////////////////////////////////////////

class Node {
public:

    virtual ~Node() = default;
    virtual double evaluate(const Environment& env) const = 0;
    virtual std::unique_ptr<Node> differentiate(const std::string& var) const = 0;
    virtual std::unique_ptr<Node> clone() const = 0;
    virtual bool isConstant(const std::string& var) const = 0;
};

/// CHILD CLASS : NUMBERNODE ///////////////////////////////////////////////////////////

class NumberNode : public Node {
    double value;
public:

    NumberNode(double v) 
        : value(v) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    bool isConstant(const std::string& var) const override;
};

/// CHILD CLASS : VARNODE ///////////////////////////////////////////////////////////

class VarNode : public Node {
    std::string name;
public:

    VarNode(std::string n) 
        : name(std::move(n)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    bool isConstant(const std::string& var) const override;


};

/// CHILD CLASS : BINARYOPNODE ///////////////////////////////////////////////////////////

class BinaryOpNode : public Node {

    BinaryOperator op;
    std::unique_ptr<Node> left, right;

public:

    BinaryOpNode(BinaryOperator o, std::unique_ptr<Node> l, std::unique_ptr<Node> r) 
        : op(o), left(std::move(l)), right(std::move(r)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    bool isConstant(const std::string& var) const override;

// operator differentiations (helper functions)

private:
    std::unique_ptr<Node> differentiateLinear(const std::string& var) const;
    std::unique_ptr<Node> differentiateProduct(const std::string& var) const;
    std::unique_ptr<Node> differentiateQuotient(const std::string& var) const;
    std::unique_ptr<Node> differentiatePower(const std::string& var) const;

    // power helpers

    std::unique_ptr<Node> differentiatePowerConstExponent(const std::string& var) const;
    std::unique_ptr<Node> differentiatePowerConstBase(const std::string& var) const;
    std::unique_ptr<Node> differentiatePowerGeneral(const std::string& var) const;

};

/// CHILD CLASS : UNARYOPNODE ///////////////////////////////////////////////////////////

class UnaryOpNode : public Node {
    UnaryOperator op;
    std::unique_ptr<Node> arg;
public:

    UnaryOpNode(UnaryOperator o, std::unique_ptr<Node> a)
        : op(o), arg(std::move(a)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    bool isConstant(const std::string& var) const override;
};

/// CHILD CLASS : FUNCNODE ///////////////////////////////////////////////////////////

class FuncNode : public Node {
    FunctionType func;
    std::unique_ptr<Node> arg;
public:

    FuncNode(FunctionType f, std::unique_ptr<Node> a) 
        : func(f), arg(std::move(a)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    bool isConstant(const std::string& var) const override;

private: 

    std::unique_ptr<Node> differentiateSin(const std::string& var) const;
    std::unique_ptr<Node> differentiateCos(const std::string& var) const;
    std::unique_ptr<Node> differentiateTan(const std::string& var) const;
    std::unique_ptr<Node> differentiateCot(const std::string& var) const;
    std::unique_ptr<Node> differentiateArcsin(const std::string& var) const;
    std::unique_ptr<Node> differentiateArccos(const std::string& var) const;
    std::unique_ptr<Node> differentiateArctan(const std::string& var) const;
    std::unique_ptr<Node> differentiateArccot(const std::string& var) const;
    std::unique_ptr<Node> differentiateSinh(const std::string& var) const;
    std::unique_ptr<Node> differentiateCosh(const std::string& var) const;
    std::unique_ptr<Node> differentiateTanh(const std::string& var) const;
    std::unique_ptr<Node> differentiateCoth(const std::string& var) const;
    std::unique_ptr<Node> differentiateAsinh(const std::string& var) const;
    std::unique_ptr<Node> differentiateAcosh(const std::string& var) const;
    std::unique_ptr<Node> differentiateAtanh(const std::string& var) const;
    std::unique_ptr<Node> differentiateAcoth(const std::string& var) const;
    std::unique_ptr<Node> differentiateLn(const std::string& var) const;
    std::unique_ptr<Node> differentiateLog(const std::string& var) const;
    std::unique_ptr<Node> differentiateSqrt(const std::string& var) const;
    std::unique_ptr<Node> differentiateAbs(const std::string& var) const;
};

