#include <gtest/gtest.h>

#include "helpers.h"
#include "format.h"
#include "parser.h"
#include "session.h"

#include <cmath>

/// EVALUATION ///

TEST(Session, EvaluateShowsAllForms) {
    Evaluation e = evaluateExpression("(x + 1)^2", {});
    ASSERT_FALSE(e.error);
    EXPECT_EQ(format(*e.parsed), "(x + 1)^2");
    EXPECT_EQ(format(*e.simplified), "(x + 1)^2");
    ASSERT_TRUE(e.expanded);
    EXPECT_EQ(format(*e.expanded), "x^2 + 2x + 1");
    EXPECT_EQ(e.assignments.variables, std::vector<std::string>{"x"});
    EXPECT_FALSE(e.value);
    EXPECT_FALSE(e.substituted);
}

TEST(Session, EvaluateParseErrorHasColumn) {
    Evaluation e = evaluateExpression("sin(x + ", {});
    ASSERT_TRUE(e.error);
    EXPECT_EQ(e.error->column, 8u);
    EXPECT_FALSE(e.parsed);
}

TEST(Session, EvaluateTooLongExpansion) {
    Evaluation e = evaluateExpression("(x + y + 1)^20", {});
    ASSERT_FALSE(e.error);
    EXPECT_FALSE(e.expanded);
}

TEST(Session, EvaluateWithAllValues) {
    Evaluation e = evaluateExpression("x^2 + y", {{"x", "3"}, {"y", "pi/2"}});
    ASSERT_TRUE(e.value);
    EXPECT_NEAR(*e.value, 9 + M_PI / 2, EPS);
}

TEST(Session, EvaluateWithoutVariables) {
    Evaluation e = evaluateExpression("2^10", {});
    ASSERT_TRUE(e.value);
    EXPECT_EQ(*e.value, 1024);
}

TEST(Session, EvaluatePartialSubstitution) {
    Evaluation e = evaluateExpression("sin(x + y)", {{"x", "2"}, {"y", " "}});
    EXPECT_FALSE(e.value);
    ASSERT_TRUE(e.substituted);
    EXPECT_EQ(format(*e.substituted), "sin(y + 2)");
}

TEST(Session, InvalidValuesAreReported) {
    Evaluation e = evaluateExpression("x + y", {{"x", "2 +"}, {"y", "x"}});
    EXPECT_EQ(e.assignments.errors.size(), 2u);
    EXPECT_EQ(e.assignments.errors["y"], "the value cannot contain a variable (x)");
    EXPECT_TRUE(e.assignments.values.empty());
    EXPECT_FALSE(e.substituted);
}

TEST(Session, ValuesOfOtherVariablesAreIgnored) {
    Evaluation e = evaluateExpression("x", {{"x", "1"}, {"z", "5"}});
    ASSERT_TRUE(e.value);
    EXPECT_EQ(*e.value, 1);
}

/// DIFFERENTIATION ///

TEST(Session, DifferentiateUpToOrder) {
    Differentiation d = differentiateExpression("x^3 - 2x", "x", 3, {});
    ASSERT_FALSE(d.error);
    ASSERT_EQ(d.derivatives.size(), 3u);
    EXPECT_EQ(format(*d.derivatives[0]), "3x^2 - 2");
    EXPECT_EQ(format(*d.derivatives[1]), "6x");
    EXPECT_EQ(format(*d.derivatives[2]), "6");
    EXPECT_TRUE(d.gradient.empty());
}

TEST(Session, DifferentiateByChosenVariable) {
    Differentiation d = differentiateExpression("x y^2", "y", 1, {});
    EXPECT_EQ(d.variable, "y");
    EXPECT_EQ(format(*d.derivatives[0]), "2x y");
}

TEST(Session, MissingVariableFallsBackToFirst) {
    EXPECT_EQ(differentiateExpression("t^2 + s", "x", 1, {}).variable, "t");
    EXPECT_EQ(differentiateExpression("5", "y", 1, {}).variable, "x");
}

TEST(Session, OrderIsClamped) {
    EXPECT_EQ(differentiateExpression("x^5", "x", 0, {}).derivatives.size(), 1u);
    EXPECT_EQ(differentiateExpression("x^5", "x", 99, {}).derivatives.size(),
              size_t(MAX_DERIVATIVE_ORDER));
}

TEST(Session, GradientForSeveralVariables) {
    Differentiation d = differentiateExpression("x^2 y + y", "x", 1, {});
    ASSERT_EQ(d.gradient.size(), 2u);
    EXPECT_EQ(d.gradient[0].first, "x");
    EXPECT_EQ(format(*d.gradient[0].second), "2x y");
    EXPECT_EQ(d.gradient[1].first, "y");
    EXPECT_EQ(format(*d.gradient[1].second), "x^2 + 1");
}

TEST(Session, DerivativeValuesAtPoint) {
    Differentiation d = differentiateExpression("x^3", "x", 2, {{"x", "2"}});
    ASSERT_TRUE(d.value);
    EXPECT_EQ(*d.value, 8);
    ASSERT_EQ(d.derivativeValues.size(), 2u);
    EXPECT_EQ(d.derivativeValues[0], 12);
    EXPECT_EQ(d.derivativeValues[1], 12);
}

TEST(Session, NonDifferentiableFunction) {
    Differentiation d = differentiateExpression("floor(x) + y", "x", 1, {});
    EXPECT_FALSE(d.diffError.empty());
    EXPECT_TRUE(d.derivatives.empty());
    EXPECT_TRUE(d.gradient.empty());
}

/// PLOTTING ///

TEST(Session, SamplesCoverBothEnds) {
    std::vector<double> s = samplePlot(*parse("x^2"), "x", {-2, 2}, 5);
    EXPECT_EQ(s, (std::vector<double>{4, 1, 0, 1, 4}));
}

TEST(Session, UndefinedSamplesAreNaN) {
    std::vector<double> s = samplePlot(*parse("sqrt(x)"), "x", {-1, 1}, 3);
    EXPECT_TRUE(std::isnan(s[0]));
    EXPECT_EQ(s[2], 1);

    for (double v : samplePlot(*parse("x + y"), "x", {0, 1}, 4)) { // y has no value
        EXPECT_TRUE(std::isnan(v));
    }
}

TEST(Session, RangeOfSmoothCurveKeepsExtremes) {
    std::optional<Range> r = plotRange(samplePlot(*parse("x^2"), "x", {-10, 10}, 101));
    ASSERT_TRUE(r);
    EXPECT_LT(r->min, 0);
    EXPECT_GT(r->max, 100);
}

TEST(Session, RangeCutsOffPoles) {
    std::optional<Range> r = plotRange(samplePlot(*parse("tan x"), "x", {-10, 10}, 160));
    ASSERT_TRUE(r);
    EXPECT_LT(r->max - r->min, 100);
}

TEST(Session, RangeOfConstantAndOfNothing) {
    std::optional<Range> r = plotRange({3, 3, 3});
    ASSERT_TRUE(r);
    EXPECT_LT(r->min, 3);
    EXPECT_GT(r->max, 3);
    EXPECT_FALSE(plotRange({NAN, INFINITY}));
}

/// NUMBERS ///

TEST(Session, FormatValue) {
    EXPECT_EQ(formatValue(std::cos(M_PI / 2)), "0");
    EXPECT_EQ(formatValue(1.0 / 0.0), "undefined");
    EXPECT_EQ(formatValue(2.5), "2.5");
}
