#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "types.h"


/// PRECEDENCE LEVELS (higher binds tighter) //////////////////////////////////////////

namespace Precedence {
    constexpr int ADDITIVE       = 1;  // + -
    constexpr int MULTIPLICATIVE = 2;  // * /
    constexpr int UNARY          = 3;  // -x
    constexpr int POWER          = 4;  // ^
    constexpr int ATOM           = 5;  // numbers, variables, function calls
}

/// PARENT CLASS : NODE /////////////////////////////////////////////////////////////////

class Node {
public:

    virtual ~Node() = default;
    virtual double evaluate(const Environment& env) const = 0;
    virtual std::unique_ptr<Node> differentiate(const std::string& var) const = 0;
    virtual std::unique_ptr<Node> clone() const = 0;

    // copy of the tree with every occurrence of var replaced by a copy of value
    virtual std::unique_ptr<Node> substitute(const std::string& var, const Node& value) const = 0;

    // appends the variables that are not in out yet, in order of first appearance
    virtual void collectVariables(std::vector<std::string>& out) const = 0;

    virtual bool isConstant(const std::string& var) const = 0;
    virtual std::string toString() const = 0;

    // structural equality: same node types, operators and values in the same shape
    virtual bool equals(const Node& other) const = 0;

    // simplifies until the tree stops changing (see simplify.cpp)
    std::unique_ptr<Node> simplify() const;

    // one bottom-up simplification pass, used by simplify()
    virtual std::unique_ptr<Node> simplifyStep() const = 0;

    // binding strength, used by toString() to decide where parentheses are needed
    virtual int precedence() const = 0;

    // direct subtrees in written order and the node's own symbol, used by drawTree()
    virtual std::vector<const Node*> children() const = 0;
    virtual std::string label() const = 0;

    // the value of the node if it is a plain number, used by simplify()
    virtual std::optional<double> numericValue() const { return std::nullopt; }
};

inline bool operator==(const Node& a, const Node& b) { return a.equals(b); }
inline bool operator!=(const Node& a, const Node& b) { return !a.equals(b); }

// variables of an expression in order of first appearance, e.g. y + x*y -> {y, x}
inline std::vector<std::string> variables(const Node& node) {
    std::vector<std::string> out;
    node.collectVariables(out);
    return out;
}

/// CHILD CLASS : NUMBERNODE ///////////////////////////////////////////////////////////

class NumberNode : public Node {
    double value;
public:

    NumberNode(double v) 
        : value(v) {}

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    std::unique_ptr<Node> substitute(const std::string& var, const Node& value) const override;
    void collectVariables(std::vector<std::string>& out) const override;
    bool isConstant(const std::string& var) const override;
    std::unique_ptr<Node> simplifyStep() const override;
    std::string toString() const override;
    bool equals(const Node& other) const override;
    int precedence() const override;
    std::vector<const Node*> children() const override;
    std::string label() const override;
    std::optional<double> numericValue() const override;
};

/// CHILD CLASS : VARNODE ///////////////////////////////////////////////////////////

class VarNode : public Node {
    std::string name;
public:

    VarNode(std::string n) 
        : name(std::move(n)) {}

    /// GETTERS ///
    const std::string& getName() const { return name; }

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    std::unique_ptr<Node> substitute(const std::string& var, const Node& value) const override;
    void collectVariables(std::vector<std::string>& out) const override;
    bool isConstant(const std::string& var) const override;
    std::unique_ptr<Node> simplifyStep() const override;
    std::string toString() const override;
    bool equals(const Node& other) const override;
    int precedence() const override;
    std::vector<const Node*> children() const override;
    std::string label() const override;


};

/// CHILD CLASS : CONSTANTNODE ///////////////////////////////////////////////////////////
// named constant like pi or e: evaluates to its value, but prints and simplifies symbolically

class ConstantNode : public Node {
    std::string name;
    double value;
public:

    ConstantNode(std::string n, double v)
        : name(std::move(n)), value(v) {}

    /// GETTERS ///
    const std::string& getName() const { return name; }

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    std::unique_ptr<Node> substitute(const std::string& var, const Node& value) const override;
    void collectVariables(std::vector<std::string>& out) const override;
    bool isConstant(const std::string& var) const override;
    std::unique_ptr<Node> simplifyStep() const override;
    std::string toString() const override;
    bool equals(const Node& other) const override;
    int precedence() const override;
    std::vector<const Node*> children() const override;
    std::string label() const override;

    // numericValue() is deliberately not overridden, so simplify() keeps 2 * pi instead of 6.28319
};

/// CHILD CLASS : BINARYOPNODE ///////////////////////////////////////////////////////////

class BinaryOpNode : public Node {

    BinaryOperator op;
    std::unique_ptr<Node> left, right;

public:

    BinaryOpNode(BinaryOperator o, std::unique_ptr<Node> l, std::unique_ptr<Node> r) 
        : op(o), left(std::move(l)), right(std::move(r)) {}

    /// GETTERS ///
    BinaryOperator getOp() const { return op; }
    const Node& getLeft() const { return *left; }
    const Node& getRight() const { return *right; }

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    std::unique_ptr<Node> substitute(const std::string& var, const Node& value) const override;
    void collectVariables(std::vector<std::string>& out) const override;
    bool isConstant(const std::string& var) const override;
    std::unique_ptr<Node> simplifyStep() const override;
    std::string toString() const override;
    bool equals(const Node& other) const override;
    int precedence() const override;
    std::vector<const Node*> children() const override;
    std::string label() const override;

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

    /// GETTERS ///
    UnaryOperator getOp() const { return op; }
    const Node& getArg() const { return *arg; }

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    std::unique_ptr<Node> substitute(const std::string& var, const Node& value) const override;
    void collectVariables(std::vector<std::string>& out) const override;
    bool isConstant(const std::string& var) const override;
    std::unique_ptr<Node> simplifyStep() const override;
    std::string toString() const override;
    bool equals(const Node& other) const override;
    int precedence() const override;
    std::vector<const Node*> children() const override;
    std::string label() const override;
};

/// CHILD CLASS : FUNCNODE ///////////////////////////////////////////////////////////

class FuncNode : public Node {
    FunctionType func;
    std::unique_ptr<Node> arg;
public:

    FuncNode(FunctionType f, std::unique_ptr<Node> a) 
        : func(f), arg(std::move(a)) {}

    /// GETTERS ///
    FunctionType getFunc() const { return func; }
    const Node& getArg() const { return *arg; }

    double evaluate(const Environment& env) const override;
    std::unique_ptr<Node> differentiate(const std::string& var) const override;
    std::unique_ptr<Node> clone() const override;
    std::unique_ptr<Node> substitute(const std::string& var, const Node& value) const override;
    void collectVariables(std::vector<std::string>& out) const override;
    bool isConstant(const std::string& var) const override;
    std::unique_ptr<Node> simplifyStep() const override;
    std::string toString() const override;
    bool equals(const Node& other) const override;
    int precedence() const override;
    std::vector<const Node*> children() const override;
    std::string label() const override;

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

