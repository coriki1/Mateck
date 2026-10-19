#include <gtest/gtest.h>

#include "helpers.h"
#include "compare.h"
#include "parser.h"

#include <random>
#include <vector>

namespace {

    int cmp(const std::string& a, const std::string& b) {
        return compare(*parse(a), *parse(b));
    }

    // small random trees to check the order properties on
    std::unique_ptr<Node> randomTree(std::mt19937& rng, int depth) {
        auto pick = [&](int n) { return std::uniform_int_distribution<int>(0, n - 1)(rng); };

        if (depth == 0 || pick(3) == 0) {
            switch (pick(4)) {
                case 0:  return num(pick(3));
                case 1:  return x();
                case 2:  return y();
                default: return parse("pi");
            }
        }
        switch (pick(4)) {
            case 0:  return neg(randomTree(rng, depth - 1));
            case 1:  return func(pick(2) ? FunctionType::SIN : FunctionType::COS, randomTree(rng, depth - 1));
            case 2:  return bin(B::MUL, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
            default: return bin(B::ADD, randomTree(rng, depth - 1), randomTree(rng, depth - 1));
        }
    }

}

TEST(Compare, KindOrder) {
    EXPECT_LT(cmp("2", "pi"), 0);          // number < constant
    EXPECT_LT(cmp("pi", "x"), 0);          // constant < variable
    EXPECT_LT(cmp("x", "sin(x)"), 0);      // variable < function
    EXPECT_LT(cmp("sin(x)", "-x"), 0);     // function < unary
    EXPECT_LT(cmp("-x", "x + 1"), 0);      // unary < binary
}

TEST(Compare, WithinKind) {
    EXPECT_LT(cmp("2", "3"), 0);
    EXPECT_LT(cmp("x", "y"), 0);
    EXPECT_LT(cmp("cos(x)", "sin(x)"), 0);   // by name
    EXPECT_LT(cmp("sin(x)", "sin(y)"), 0);   // then by argument
    EXPECT_LT(cmp("x + 1", "x + 2"), 0);     // then children left to right
}

TEST(Compare, ZeroExactlyForEqualTrees) {
    EXPECT_EQ(cmp("x + 1", "x + 1"), 0);
    EXPECT_NE(cmp("x + 1", "1 + x"), 0);
}

// sorting with compare() is only safe if it is a strict total order consistent with ==
TEST(Compare, IsTotalOrderConsistentWithEquals) {
    std::mt19937 rng(2026);
    std::vector<std::unique_ptr<Node>> trees;
    for (int i = 0; i < 60; i++) {
        trees.push_back(randomTree(rng, 3));
    }

    for (const auto& a : trees) {
        for (const auto& b : trees) {
            int ab = compare(*a, *b);
            int ba = compare(*b, *a);
            ASSERT_EQ(ab, -ba) << a->toString() << " vs " << b->toString();             // antisymmetric
            ASSERT_EQ(ab == 0, *a == *b) << a->toString() << " vs " << b->toString();   // 0 only for equal trees

            for (const auto& c : trees) {                                               // transitive
                if (ab < 0 && compare(*b, *c) < 0) {
                    ASSERT_LT(compare(*a, *c), 0) << a->toString() << ", " << b->toString() << ", " << c->toString();
                }
            }
        }
    }
}

/// ORDER IN SIMPLIFY ///

TEST(Compare, SimplifyOrdersTermsByDegree) {
    EXPECT_EQ(parse("-2x + x^3")->simplify()->toString(), "x^3 - 2 * x");
    EXPECT_EQ(parse("1 + x + x^2")->simplify()->toString(), "x^2 + x + 1");
    EXPECT_EQ(parse("x + 1/x")->simplify()->toString(), "x + 1 / x");
}

TEST(Compare, SimplifyOrdersEqualDegreeByVariable) {
    EXPECT_EQ(parse("y^2 + x y + x^2")->simplify()->toString(), "x^2 + x * y + y^2");
    EXPECT_EQ(parse("y + x")->simplify()->toString(), "x + y");
}

TEST(Compare, SimplifyOrdersFactors) {
    EXPECT_EQ(parse("sin(x) * x * 2")->simplify()->toString(), "2 * x * sin(x)");
    EXPECT_EQ(parse("x pi")->simplify()->toString(), "pi * x");
    EXPECT_EQ(parse("(x + 1) x")->simplify()->toString(), "x * (x + 1)");
}
