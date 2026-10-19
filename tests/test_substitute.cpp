#include <gtest/gtest.h>

#include "helpers.h"
#include "parser.h"

/// VARIABLES ///

TEST(Variables, InOrderOfFirstAppearance) {
    EXPECT_EQ(variables(*parse("y + x")), (std::vector<std::string>{"y", "x"}));
    EXPECT_EQ(variables(*parse("x * x + sin(x y)")), (std::vector<std::string>{"x", "y"}));
    EXPECT_EQ(variables(*parse("-z^2 / w")), (std::vector<std::string>{"z", "w"}));
    EXPECT_EQ(variables(*parse("sin^2 t")), (std::vector<std::string>{"t"}));
}

TEST(Variables, ConstantsAreNotVariables) {
    EXPECT_TRUE(variables(*parse("2 pi e")).empty());
    EXPECT_TRUE(variables(*parse("sqrt 2")).empty());
}

/// SUBSTITUTE ///

TEST(Substitute, ReplacesEveryOccurrence) {
    EXPECT_EQ(parse("x^2 + x")->substitute("x", *num(3))->toString(), "3^2 + 3");
    EXPECT_EQ(parse("-sin(x)")->substitute("x", *y())->toString(), "-sin(y)");
}

TEST(Substitute, LeavesOtherVariablesAndConstants) {
    EXPECT_EQ(parse("x + y + pi")->substitute("x", *num(2))->toString(), "2 + y + pi");
    EXPECT_EQ(parse("y")->substitute("x", *num(2))->toString(), "y");
}

TEST(Substitute, KeepsSubstitutedExpressionSymbolic) {
    EXPECT_EQ(parse("sin(x + y)")->substitute("x", *parse("pi/2"))->toString(), "sin(pi / 2 + y)");
}

TEST(Substitute, ParenthesizesWhereNeeded) {
    EXPECT_EQ(parse("x^2")->substitute("x", *parse("y + 1"))->toString(), "(y + 1)^2");
    EXPECT_EQ(parse("2x")->substitute("x", *parse("-y"))->toString(), "2 * -y");
}

TEST(Substitute, DoesNotModifyOriginal) {
    auto original = parse("x + 1");
    original->substitute("x", *num(5));
    EXPECT_EQ(original->toString(), "x + 1");
}

TEST(Substitute, PartialThenSimplify) {
    EXPECT_EQ(parse("sin(x + y)")->substitute("x", *num(2))->simplify()->toString(), "sin(y + 2)");
    EXPECT_EQ(parse("x y + x")->substitute("x", *num(3))->simplify()->toString(), "3 * y + 3");
}

TEST(Substitute, AllVariablesThenEvaluate) {
    auto expr = parse("x^2 + y")->substitute("x", *num(3))->substitute("y", *num(1));
    EXPECT_TRUE(variables(*expr).empty());
    EXPECT_NEAR(expr->evaluate({}), 10.0, EPS);
}
