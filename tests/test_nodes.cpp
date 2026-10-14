#include <gtest/gtest.h>

#include "helpers.h"
#include "errors.h"

#include <cmath>

namespace {
    const Environment X3 = {{"x", 3.0}};
}

/// EVALUATE / DIFFERENTIATE ///

TEST(NumberNode, Basics) {
    NumberNode n(5.0);
    EXPECT_NEAR(n.evaluate(X3), 5.0, EPS);
    EXPECT_TRUE(n.isConstant("x"));
    EXPECT_NEAR(n.differentiate("x")->evaluate(X3), 0.0, EPS);
}

TEST(VarNode, Basics) {
    VarNode v("x");
    EXPECT_NEAR(v.evaluate(X3), 3.0, EPS);
    EXPECT_FALSE(v.isConstant("x"));
    EXPECT_TRUE(v.isConstant("y"));
    EXPECT_NEAR(v.differentiate("x")->evaluate(X3), 1.0, EPS);
}

// (x + 2) * (x - 1), x = 3 -> 5 * 2 = 10
TEST(BinaryOpNode, Evaluate) {
    auto expr = bin(B::MUL, bin(B::ADD, x(), num(2)), bin(B::SUB, x(), num(1)));
    EXPECT_NEAR(expr->evaluate(X3), 10.0, EPS);
}

// x^2 -> 2x = 10 at x = 5
TEST(Differentiate, PowerRule) {
    auto square = bin(B::POW, x(), num(2));
    EXPECT_NEAR(square->differentiate("x")->evaluate({{"x", 5.0}}), 10.0, EPS);
}

// x * x -> 2x = 10 at x = 5
TEST(Differentiate, ProductRule) {
    auto product = bin(B::MUL, x(), x());
    EXPECT_NEAR(product->differentiate("x")->evaluate({{"x", 5.0}}), 10.0, EPS);
}

// x / (x + 1) -> 1 / (x + 1)^2 = 1/9 at x = 2
TEST(Differentiate, QuotientRule) {
    auto quotient = bin(B::DIV, x(), bin(B::ADD, x(), num(1)));
    EXPECT_NEAR(quotient->differentiate("x")->evaluate({{"x", 2.0}}), 1.0 / 9.0, EPS);
}

// used to be NaN: 0 * inf
TEST(Differentiate, ConstantSubtreeGivesZeroNotNaN) {
    const Environment env = {{"x", 2.0}, {"y", 0.0}};

    EXPECT_NEAR(func(FunctionType::SQRT, y())->differentiate("x")->evaluate(env), 0.0, EPS);
    EXPECT_NEAR(func(FunctionType::LN, y())->differentiate("x")->evaluate(env), 0.0, EPS);
    EXPECT_NEAR(bin(B::POW, num(0), y())->differentiate("x")->evaluate(env), 0.0, EPS);

    // x * sqrt(y) -> sqrt(y) = 0
    auto xTimesSqrtY = bin(B::MUL, x(), func(FunctionType::SQRT, y()));
    EXPECT_NEAR(xTimesSqrtY->differentiate("x")->evaluate(env), 0.0, EPS);
}

TEST(Clone, EvaluatesLikeOriginal) {
    auto original = bin(B::ADD, x(), num(1));
    EXPECT_NEAR(original->clone()->evaluate(X3), original->evaluate(X3), EPS);
}

// range (0, pi)
TEST(FuncNode, ArccotRange) {
    EXPECT_NEAR(func(FunctionType::ARCCOT, num(-1))->evaluate({}), 3.0 * std::atan(1.0), EPS);
    EXPECT_NEAR(func(FunctionType::ARCCOT, num(0))->evaluate({}), 2.0 * std::atan(1.0), EPS);
}

/// TOSTRING ///

TEST(ToString, ParenthesesFollowPrecedence) {
    EXPECT_EQ(bin(B::MUL, bin(B::ADD, x(), num(2)), bin(B::SUB, x(), num(1)))->toString(), "(x + 2) * (x - 1)");
    EXPECT_EQ(bin(B::SUB, x(), bin(B::SUB, x(), num(1)))->toString(), "x - (x - 1)");
    EXPECT_EQ(bin(B::POW, bin(B::POW, x(), num(2)), num(3))->toString(), "(x^2)^3");
    EXPECT_EQ(bin(B::POW, num(-1), num(2))->toString(), "(-1)^2");
    EXPECT_EQ(neg(bin(B::POW, x(), num(2)))->toString(), "-x^2");
    EXPECT_EQ(func(FunctionType::SIN, bin(B::ADD, x(), num(1)))->toString(), "sin(x + 1)");
}

/// EQUALS ///

TEST(Equals, IsStructural) {
    EXPECT_TRUE(*bin(B::ADD, x(), num(1)) == *bin(B::ADD, x(), num(1)));
    EXPECT_TRUE(*bin(B::ADD, x(), num(1)) != *bin(B::ADD, num(1), x()));   // same value, different shape
    EXPECT_FALSE(*bin(B::ADD, x(), num(1)) == *bin(B::SUB, x(), num(1)));
    EXPECT_FALSE(VarNode("x") == VarNode("y"));
    EXPECT_TRUE(NumberNode(2) == NumberNode(2));
    EXPECT_FALSE(*func(FunctionType::SIN, x()) == *func(FunctionType::COS, x()));
    EXPECT_FALSE(NumberNode(2) == VarNode("x"));
}

/// ERRORS ///

TEST(Errors, UndefinedVariableThrowsEvalError) {
    EXPECT_THROW(VarNode("z").evaluate({}), EvalError);
}

TEST(Errors, DifferentiatingFloorThrowsDiffError) {
    EXPECT_THROW(func(FunctionType::FLOOR, x())->differentiate("x"), DiffError);
}

/// NAME LOOKUPS ///

TEST(Names, FunctionsAndConstants) {
    EXPECT_EQ(functionTypeToString(FunctionType::ARCSIN), "arcsin");
    EXPECT_TRUE(stringToFunctionType("acoth") == FunctionType::ACOTH);
    EXPECT_FALSE(stringToFunctionType("foo").has_value());
    EXPECT_NEAR(*constantValue("pi"), std::acos(-1.0), EPS);
}
