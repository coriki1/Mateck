#include <gtest/gtest.h>

#include "helpers.h"
#include "expand.h"
#include "format.h"
#include "parser.h"

#include <chrono>
#include <cmath>
#include <random>

namespace {

    // expanded form as the REPL shows it, or "" if it was too long
    std::string expanded(const std::string& input) {
        std::unique_ptr<Node> result = expand(*parse(input));
        return result ? format(*result) : "";
    }

    std::unique_ptr<Node> randomTree(std::mt19937& rng, int depth) {
        auto pick = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(rng); };

        if (depth == 0 || pick(4) == 0) {
            switch (pick(4)) {
                case 0:  return num(pick(7) - 3);
                case 1:  return x();
                case 2:  return y();
                default: return num(0.5);
            }
        }
        switch (pick(8)) {
            case 0:  return neg(randomTree(rng, depth - 1));
            case 1:  return func(FunctionType::SIN, randomTree(rng, depth - 1));
            case 2:  return bin(B::POW, randomTree(rng, depth - 1), num(pick(4)));
            case 3:  return bin(B::DIV, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
            case 4:
            case 5:  return bin(B::MUL, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
            default: return bin(pick(2) ? B::ADD : B::SUB, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
        }
    }

}

TEST(Expand, MultipliesOutProducts) {
    EXPECT_EQ(expanded("(x + 1)(x - 1)"), "x^2 - 1");
    EXPECT_EQ(expanded("2(x + 1)"), "2x + 2");
    EXPECT_EQ(expanded("-(x - y)"), "-x + y");
    EXPECT_EQ(expanded("(x + y)(a + b)"), "a x + a y + b x + b y");
    EXPECT_EQ(expanded("x(x + 1) - x^2"), "x");
}

TEST(Expand, WritesOutPowers) {
    EXPECT_EQ(expanded("(x + 1)^2"), "x^2 + 2x + 1");
    EXPECT_EQ(expanded("(x + 1)^3"), "x^3 + 3x^2 + 3x + 1");
    EXPECT_EQ(expanded("(x - y)^2"), "x^2 - 2x y + y^2");
    EXPECT_EQ(expanded("(x + 1)^(1 + 1)"), "x^2 + 2x + 1");
    EXPECT_EQ(expanded("2(x + 1)^1"), "2x + 2");
}

TEST(Expand, InsideFunctions) {
    EXPECT_EQ(expanded("sin((x + 1)^2)"), "sin(x^2 + 2x + 1)");
    EXPECT_EQ(expanded("2sin(x)(x + 1)"), "2x sin(x) + 2sin(x)");
}

TEST(Expand, LeavesWhatCannotBeExpanded) {
    EXPECT_EQ(expanded("x/(x + 1)"), "x/(x + 1)");          // denominators stay whole
    EXPECT_EQ(expanded("(x + 1)/x"), "1/x + 1");             // but numerators are split
    EXPECT_EQ(expanded("(x + 1)^-1"), "(x + 1)^(-1)");
    EXPECT_EQ(expanded("(x + 1)^(1/2)"), "(x + 1)^(1/2)");
    EXPECT_EQ(expanded("(x + 1)^y"), "(x + 1)^y");
    EXPECT_EQ(expanded("x^3 - 2x"), "x^3 - 2x");             // nothing to expand
}

TEST(Expand, TooLongGivesUp) {
    EXPECT_NE(expanded("(x + 1)^49"), "");                  // 50 terms: still fits
    EXPECT_EQ(expanded("(x + 1)^50"), "");                  // 51 terms: too long
    EXPECT_EQ(expanded("(x + 1)^60"), "");
    EXPECT_EQ(expanded("(x + y + z)^10"), "");               // 66 terms
}

TEST(Expand, HugePowerStopsQuickly) {
    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(expanded("(x + 1)^1000000"), "");
    EXPECT_EQ(expanded("(x + 1)^200"), "");
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
}

TEST(Expand, IsIdempotent) {
    auto once = expand(*parse("(x + 2)^3 (x - y)"));
    ASSERT_TRUE(once);
    EXPECT_TRUE(*expand(*once) == *once) << format(*once);
}

// expanding must not change the value
TEST(Expand, KeepsValue) {
    std::mt19937 rng(7);
    const Environment env = {{"x", 0.7}, {"y", -1.3}};
    int checked = 0;

    for (int i = 0; i < 2000; i++) {
        auto tree = randomTree(rng, 4);
        auto result = expand(*tree);
        if (!result) {
            continue;
        }

        double a = tree->evaluate(env);
        double b = result->evaluate(env);
        if (!std::isfinite(a) || std::abs(a) > 1e6) {
            continue;
        }
        checked++;
        EXPECT_NEAR(a, b, 1e-6 * std::max(1.0, std::abs(a))) << tree->toString() << "\n  -> " << result->toString();
    }

    EXPECT_GT(checked, 1000);
}
