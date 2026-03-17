#pragma once

#include <iostream>
#include <memory>
#include "op_func.h"


class Node {
public:
    virtual ~Node() {}
    virtual double evaluate() const = 0;
};

class NumberNode : public Node {
    double value;
public:
    NumberNode();
    NumberNode(double v);

    double evaluate() const override;
};

class BinaryOpNode : public Node {
    BINARYOP op;
    std::unique_ptr<Node> left, right;
public:
    double evaluate() const override;
};

class UnaryOpNode : public Node {
    UNARYOP op;
    std::unique_ptr<Node> arg;
public:
    double evaluate() const override;
};

class FuncNode : public Node {
    FUNC func;
    std::unique_ptr<Node> arg;
public:
    double evaluate() const override;
};