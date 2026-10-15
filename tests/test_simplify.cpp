#include <gtest/gtest.h>

#include "helpers.h"

/// IDENTITIES AND CONSTANT FOLDING ///

TEST(Simplify, Identities) {
    EXPECT_EQ(simp(bin(B::ADD, num(0), x())), "x");
    EXPECT_EQ(simp(bin(B::SUB, x(), num(0))), "x");
    EXPECT_EQ(simp(bin(B::SUB, num(0), x())), "-x");
    EXPECT_EQ(simp(bin(B::MUL, num(1), x())), "x");
    EXPECT_EQ(simp(bin(B::MUL, x(), num(0))), "0");
    EXPECT_EQ(simp(bin(B::MUL, num(-1), x())), "-x");
    EXPECT_EQ(simp(bin(B::DIV, num(0), x())), "0");
    EXPECT_EQ(simp(bin(B::DIV, x(), num(1))), "x");
    EXPECT_EQ(simp(bin(B::POW, x(), num(0))), "1");
    EXPECT_EQ(simp(bin(B::POW, x(), num(1))), "x");
    EXPECT_EQ(simp(bin(B::POW, num(1), x())), "1");
    EXPECT_EQ(simp(neg(neg(x()))), "x");
}

TEST(Simplify, FoldsConstants) {
    EXPECT_EQ(simp(bin(B::ADD, bin(B::MUL, num(2), num(3)), x())), "x + 6");
    EXPECT_EQ(simp(func(FunctionType::SIN, num(0))), "0");
}

// folding would produce inf or NaN
TEST(Simplify, KeepsNonFiniteConstants) {
    EXPECT_EQ(simp(bin(B::DIV, num(1), num(0))), "1 / 0");
    EXPECT_EQ(simp(bin(B::DIV, num(0), num(0))), "0 / 0");
    EXPECT_EQ(simp(func(FunctionType::LN, num(0))), "ln(0)");
}

TEST(Simplify, DoesNotModifyOriginal) {
    auto original = bin(B::MUL, num(1), x());
    original->simplify();
    EXPECT_EQ(original->toString(), "1 * x");
}

/// DERIVATIVES ///

TEST(Simplify, Derivatives) {
    EXPECT_EQ(bin(B::POW, x(), num(2))->differentiate("x")->simplify()->toString(), "2 * x");
    EXPECT_EQ(bin(B::POW, x(), num(3))->differentiate("x")->simplify()->toString(), "3 * x^2");
    EXPECT_EQ(func(FunctionType::SIN, x())->differentiate("x")->simplify()->toString(), "cos(x)");
    EXPECT_EQ(func(FunctionType::COS, x())->differentiate("x")->simplify()->toString(), "-sin(x)");
}

// used to be NaN at x = 0: 0 * x^(-1) * 1
TEST(Simplify, DerivativeOfXToTheZeroIsZero) {
    auto derivative = bin(B::POW, x(), num(0))->differentiate("x")->simplify();
    EXPECT_NEAR(derivative->evaluate({{"x", 0.0}}), 0.0, EPS);
}

/// FLATTENING AND MERGING ///

TEST(Simplify, MergesAcrossNestedChains) {
    EXPECT_EQ(simp(bin(B::SUB, bin(B::ADD, x(), num(1)), x())), "1");
    EXPECT_EQ(simp(bin(B::MUL, bin(B::ADD, x(), num(1)), bin(B::ADD, x(), num(1)))), "(x + 1)^2");
    EXPECT_EQ(simp(bin(B::DIV, num(3), bin(B::MUL, num(3), x()))), "1 / x");
    EXPECT_EQ(simp(bin(B::MUL, num(2), bin(B::MUL, num(3), x()))), "6 * x");
}

TEST(Simplify, MergesLikeTerms) {
    EXPECT_EQ(simp(bin(B::ADD, x(), x())), "2 * x");
    EXPECT_EQ(simp(bin(B::SUB, x(), x())), "0");
    EXPECT_EQ(simp(bin(B::ADD, bin(B::MUL, num(2), x()), bin(B::MUL, num(3), x()))), "5 * x");
    EXPECT_EQ(simp(bin(B::SUB, bin(B::MUL, x(), y()), bin(B::MUL, y(), x()))), "0");
}

TEST(Simplify, MergesLikeFactors) {
    EXPECT_EQ(simp(bin(B::MUL, x(), bin(B::POW, x(), num(2)))), "x^3");
    EXPECT_EQ(simp(bin(B::DIV, x(), x())), "1");
    EXPECT_EQ(simp(bin(B::DIV, bin(B::POW, x(), num(2)), x())), "x");
}

TEST(Simplify, KeepsCoefficientsAsFractions) {
    EXPECT_EQ(simp(bin(B::ADD, bin(B::DIV, x(), num(3)), bin(B::DIV, x(), num(3)))), "2 * x / 3");
    EXPECT_EQ(simp(bin(B::DIV, bin(B::MUL, num(6), x()), num(4))), "3 * x / 2");
}

TEST(Simplify, KeepsNumberFractionsExact) {
    EXPECT_EQ(simp(bin(B::DIV, num(1), num(2))), "1 / 2");
    EXPECT_EQ(simp(bin(B::DIV, num(6), num(4))), "3 / 2");
    EXPECT_EQ(simp(bin(B::DIV, num(6), num(3))), "2");
    EXPECT_EQ(simp(bin(B::MUL, bin(B::DIV, num(1), num(2)), x())), "x / 2");
    EXPECT_EQ(simp(bin(B::ADD, bin(B::DIV, num(1), num(2)), bin(B::DIV, num(1), num(3)))), "5 / 6");
    EXPECT_EQ(simp(bin(B::SUB, x(), bin(B::DIV, num(1), num(2)))), "x - 1 / 2");
}

/// CANONICAL FORM ///

TEST(Simplify, CanonicalOrderAndSigns) {
    EXPECT_EQ(simp(bin(B::ADD, num(1), x())), "x + 1");
    EXPECT_EQ(simp(bin(B::ADD, y(), x())), "x + y");
    EXPECT_EQ(simp(bin(B::MUL, y(), x())), "x * y");
    EXPECT_EQ(simp(bin(B::SUB, num(1), x())), "-x + 1");
    EXPECT_EQ(simp(neg(bin(B::ADD, x(), num(1)))), "-x - 1");
    EXPECT_EQ(simp(neg(bin(B::MUL, num(2), x()))), "-2 * x");
    EXPECT_EQ(simp(bin(B::MUL, x(), neg(y()))), "-(x * y)");
}

// 2 * x + (y + 1) and 1 + (y + x * 2)
TEST(Simplify, EqualExpressionsGetTheSameCanonicalForm) {
    auto a = bin(B::ADD, bin(B::MUL, num(2), x()), bin(B::ADD, y(), num(1)));
    auto b = bin(B::ADD, num(1), bin(B::ADD, y(), bin(B::MUL, x(), num(2))));
    EXPECT_TRUE(*a->simplify() == *b->simplify())
        << a->simplify()->toString() << " vs " << b->simplify()->toString();
}

TEST(Simplify, IsIdempotentAndKeepsValue) {
    auto messy = bin(B::SUB,
        bin(B::DIV, bin(B::MUL, num(-2), bin(B::POW, x(), num(3))), bin(B::MUL, num(4), y())),
        bin(B::ADD, neg(x()), bin(B::DIV, num(1), y())));
    auto once = messy->simplify();

    EXPECT_TRUE(*once->simplify() == *once) << once->toString();

    const Environment env = {{"x", 1.7}, {"y", -0.4}};
    EXPECT_NEAR(once->evaluate(env), messy->evaluate(env), EPS);
}
