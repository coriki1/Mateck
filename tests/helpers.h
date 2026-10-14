#pragma once

#include "node.h"

#include <memory>
#include <string>

/// SHARED TEST HELPERS: short builders for expression trees ///

constexpr double EPS = 1e-9;

using B = BinaryOperator;

inline std::unique_ptr<Node> x() { return std::make_unique<VarNode>("x"); }
inline std::unique_ptr<Node> y() { return std::make_unique<VarNode>("y"); }
inline std::unique_ptr<Node> num(double v) { return std::make_unique<NumberNode>(v); }

inline std::unique_ptr<Node> bin(BinaryOperator op, std::unique_ptr<Node> l, std::unique_ptr<Node> r) {
    return std::make_unique<BinaryOpNode>(op, std::move(l), std::move(r));
}

inline std::unique_ptr<Node> neg(std::unique_ptr<Node> a) {
    return std::make_unique<UnaryOpNode>(UnaryOperator::SUB, std::move(a));
}

inline std::unique_ptr<Node> func(FunctionType f, std::unique_ptr<Node> a) {
    return std::make_unique<FuncNode>(f, std::move(a));
}

// simplified form as text, e.g. simp(x + 0) == "x"
inline std::string simp(const std::unique_ptr<Node>& n) {
    return n->simplify()->toString();
}
