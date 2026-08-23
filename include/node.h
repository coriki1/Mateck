#pragma once

#include <iostream>
#include <memory>

#include "types.h"

class Node {
public:

    virtual ~Node() = default;
    virtual double evaluate(const Environment& env) const = 0;
    virtual std::unique_ptr<Node> differentiate() const = 0;
};

class NumberNode : public Node {
    double value;
public:

    NumberNode(double v) 
        : value(v) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate() const override;
};

class VarNode : public Node {
    std::string name;
public:

    VarNode(std::string n) 
        : name(std::move(n)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate() const override;
};

class BinaryOpNode : public Node {
    BinaryOperator op;
    std::unique_ptr<Node> left, right;
public:

    BinaryOpNode(BinaryOperator o, std::unique_ptr<Node> l, std::unique_ptr<Node> r) 
        : op(o), left(std::move(l)), right(std::move(r)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate() const override;
};

class UnaryOpNode : public Node {
    UnaryOperator op;
    std::unique_ptr<Node> arg;
public:

    UnaryOpNode(UnaryOperator o, std::unique_ptr<Node> a)
        : op(o), arg(std::move(a)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate() const override;
};

class FuncNode : public Node {
    FunctionType func;
    std::unique_ptr<Node> arg;
public:

    FuncNode(FunctionType f, std::unique_ptr<Node> a) 
        : func(f), arg(std::move(a)) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate() const override;
};

