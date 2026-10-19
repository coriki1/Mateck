#include <gtest/gtest.h>

#include "helpers.h"
#include "format.h"
#include "parser.h"

#include <cmath>
#include <random>

namespace {

    std::string fmt(const std::string& input) {
        return format(*parse(input));
    }

    std::string fmtSimplified(const std::string& input) {
        return format(*parse(input)->simplify());
    }

    std::unique_ptr<Node> randomTree(std::mt19937& rng, int depth) {
        auto pick = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(rng); };

        if (depth == 0 || pick(4) == 0) {
            switch (pick(5)) {
                case 0:  return num(pick(9) - 4);
                case 1:  return num(0.5);
                case 2:  return x();
                case 3:  return y();
                default: return parse("pi");
            }
        }
        switch (pick(9)) {
            case 0:  return neg(randomTree(rng, depth - 1));
            case 1:  return func(FunctionType::SIN, randomTree(rng, depth - 1));
            case 2:  return bin(B::ADD, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
            case 3:  return bin(B::SUB, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
            case 4:
            case 5:  return bin(B::MUL, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
            case 6:  return bin(B::DIV, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
            default: return bin(B::POW, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
        }
    }

}

/// NUMBERS ///

TEST(Format, Numbers) {
    EXPECT_EQ(formatNumber(2), "2");
    EXPECT_EQ(formatNumber(-0.0), "0");
    EXPECT_EQ(formatNumber(0.5), "0.5");
    EXPECT_EQ(formatNumber(2 * std::acos(-1.0)), "6.283185307");    // 10 significant digits
    EXPECT_EQ(formatNumber(1e20), "100000000000000000000");         // no scientific notation
    EXPECT_EQ(formatNumber(1e-12), "0.000000000001");
}

/// IMPLICIT MULTIPLICATION ///

TEST(Format, ImplicitMultiplication) {
    EXPECT_EQ(fmt("2 * x"), "2x");
    EXPECT_EQ(fmt("2 * x^2"), "2x^2");
    EXPECT_EQ(fmt("2 * sin(x)"), "2sin(x)");
    EXPECT_EQ(fmt("2 * (x + 1)"), "2(x + 1)");
    EXPECT_EQ(fmt("(x + 1) * (x - 1)"), "(x + 1)(x - 1)");
    EXPECT_EQ(fmt("2 * pi"), "2pi");
}

// without a space neighbouring names could merge: p*i -> "pi", a*sinh(x) -> "asinh(x)"
TEST(Format, SpaceBetweenNames) {
    EXPECT_EQ(fmt("x * y"), "x y");
    EXPECT_EQ(fmt("p * i"), "p i");
    EXPECT_EQ(fmt("a * sinh(x)"), "a sinh(x)");
    EXPECT_EQ(fmt("x^2 * y"), "x^2 y");
    EXPECT_EQ(fmt("2 * x * cos(x)"), "2x cos(x)");
}

TEST(Format, KeepsTimesWhereNeeded) {
    EXPECT_EQ(fmt("2 * 3"), "2 * 3");      // "23" would be one number
    EXPECT_EQ(fmt("x * 3"), "x * 3");      // "x 3" is a parse error
    EXPECT_EQ(fmt("x * -y"), "x * -y");    // "x -y" would be a subtraction
}

/// FRACTIONS AND SIGNS ///

TEST(Format, CompactFractions) {
    EXPECT_EQ(fmt("x / 2"), "x/2");
    EXPECT_EQ(fmt("5 * x / 6"), "5x/6");
    EXPECT_EQ(fmt("x - 1 / 2"), "x - 1/2");
    EXPECT_EQ(fmt("1 / (2 * sqrt(x))"), "1/(2sqrt(x))");
    EXPECT_EQ(fmt("(x + 1) / x^2"), "(x + 1)/x^2");
}

TEST(Format, Signs) {
    EXPECT_EQ(fmt("-(x * y)"), "-x y");
    EXPECT_EQ(fmt("-(x + 1)"), "-(x + 1)");
    EXPECT_EQ(fmt("-(-x)"), "-(-x)");
    EXPECT_EQ(fmt("-x^2"), "-x^2");
    EXPECT_EQ(fmt("x^(-1)"), "x^(-1)");
    EXPECT_EQ(fmt("(-2)^2"), "(-2)^2");
}

/// SIMPLIFIED OUTPUT (what the REPL shows) ///

TEST(Format, SimplifiedExamples) {
    EXPECT_EQ(fmtSimplified("x^3 - 2x"), "x^3 - 2x");
    EXPECT_EQ(fmtSimplified("-2x + x^3"), "x^3 - 2x");
    EXPECT_EQ(fmtSimplified("1/2x + 1/3x"), "5x/6");
    EXPECT_EQ(fmtSimplified("x y + x"), "x y + x");
    EXPECT_EQ(format(*parse("sin x^2")->differentiate("x")->simplify()), "2x cos(x^2)");
    EXPECT_EQ(format(*parse("sqrt x")->differentiate("x")->simplify()), "1/(2sqrt(x))");
}

/// ROUND TRIP ///

// The formatted text must mean the same expression. Compared as formatted canonical forms,
// because numbers are written with 10 significant digits, so sin(1) = 0.8414709848078965
// comes back as 0.8414709848.
TEST(Format, ParsesBackToTheSameExpression) {
    std::mt19937 rng(42);
    int checked = 0;

    for (int i = 0; i < 3000; i++) {
        auto tree = randomTree(rng, 4);
        auto simplified = tree->simplify();

        // simplify() leaves products with a division by 0 as they are, so they have no canonical form
        if (simplified->toString().find("/ 0") != std::string::npos) {
            continue;
        }
        checked++;

        std::string expected = format(*simplified);

        for (const std::string& text : {format(*tree), expected}) {
            std::unique_ptr<Node> reparsed;
            ASSERT_NO_THROW(reparsed = parse(text)) << text;
            EXPECT_EQ(format(*reparsed->simplify()), expected) << "formatted: " << text;
        }
    }

    EXPECT_GT(checked, 2000);
}
