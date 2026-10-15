#include <gtest/gtest.h>

#include "helpers.h"
#include "parser.h"
#include "errors.h"

#include <cmath>

namespace {

    // the parsed tree printed back, e.g. tree("2x") == "2 * x"
    std::string tree(const std::string& input) {
        return parse(input)->toString();
    }

    double value(const std::string& input, const Environment& env = {}) {
        return parse(input)->evaluate(env);
    }

    // simplified derivative by x, e.g. derivative("x^2") == "2 * x"
    std::string derivative(const std::string& input) {
        return parse(input)->differentiate("x")->simplify()->toString();
    }

    // position of the ParseError, or -1 if parsing succeeded
    long errorPosition(const std::string& input) {
        try {
            parse(input);
        }
        catch (const ParseError& e) {
            return static_cast<long>(e.getPosition());
        }
        return -1;
    }

    std::string errorMessage(const std::string& input) {
        try {
            parse(input);
        }
        catch (const ParseError& e) {
            return e.what();
        }
        return "";
    }

}

/// BASICS ///

TEST(Parser, Atoms) {
    EXPECT_EQ(tree("42"), "42");
    EXPECT_EQ(tree(".5"), "0.5");
    EXPECT_EQ(tree("x"), "x");
    EXPECT_EQ(tree("pi"), "pi");
    EXPECT_EQ(tree("(x)"), "x");
}

TEST(Parser, Precedence) {
    EXPECT_NEAR(value("1 + 2 * 3"), 7.0, EPS);
    EXPECT_NEAR(value("(1 + 2) * 3"), 9.0, EPS);
    EXPECT_NEAR(value("10 - 4 - 3"), 3.0, EPS);    // left-associative
    EXPECT_NEAR(value("24 / 4 / 2"), 3.0, EPS);    // left-associative
    EXPECT_NEAR(value("2 * 3^2"), 18.0, EPS);
}

TEST(Parser, PowerIsRightAssociative) {
    EXPECT_EQ(tree("2^3^2"), "2^3^2");
    EXPECT_NEAR(value("2^3^2"), 512.0, EPS);
}

TEST(Parser, UnaryMinusIsWeakerThanPower) {
    EXPECT_EQ(tree("-x^2"), "-x^2");
    EXPECT_NEAR(value("-x^2", {{"x", 3.0}}), -9.0, EPS);
    EXPECT_NEAR(value("-2^2"), -4.0, EPS);
    EXPECT_NEAR(value("2^-1"), 0.5, EPS);
    EXPECT_NEAR(value("--3"), 3.0, EPS);
    EXPECT_EQ(tree("+x"), "+x");
    EXPECT_NEAR(value("2 * -3"), -6.0, EPS);
}

/// IMPLICIT MULTIPLICATION ///

TEST(Parser, ImplicitMultiplication) {
    EXPECT_EQ(tree("2x"), "2 * x");
    EXPECT_EQ(tree("xy"), "x * y");
    EXPECT_EQ(tree("2sin(x)"), "2 * sin(x)");
    EXPECT_EQ(tree("(x+1)(x-1)"), "(x + 1) * (x - 1)");
    EXPECT_EQ(tree("x(x+1)"), "x * (x + 1)");
    EXPECT_EQ(tree("2pi"), "2 * pi");
}

// binds like *, so 1/2x = (1/2) * x
TEST(Parser, ImplicitMultiplicationHasSamePrecedenceAsTimes) {
    EXPECT_EQ(tree("1/2x"), "1 / 2 * x");
    EXPECT_NEAR(value("1/2x", {{"x", 4.0}}), 2.0, EPS);
    EXPECT_EQ(tree("2^3x"), "2^3 * x");
    EXPECT_NEAR(value("2^3x", {{"x", 1.0}}), 8.0, EPS);
}

/// FUNCTIONS ///

TEST(Parser, FunctionWithParentheses) {
    EXPECT_EQ(tree("sin(x)"), "sin(x)");
    EXPECT_EQ(tree("sin(x)^2"), "sin(x)^2");
    EXPECT_EQ(tree("sin (x+1)^2"), "sin(x + 1)^2");    // parentheses right after the name are the argument
    EXPECT_EQ(tree("ln(sqrt(x))"), "ln(sqrt(x))");
}

TEST(Parser, FunctionWithoutParentheses) {
    EXPECT_EQ(tree("sin x^2"), "sin(x^2)");
    EXPECT_EQ(tree("sin 2x"), "sin(2 * x)");
    EXPECT_EQ(tree("sin x + 1"), "sin(x) + 1");
    EXPECT_EQ(tree("sin x * y"), "sin(x) * y");
    EXPECT_EQ(tree("sin x / 2"), "sin(x) / 2");
    EXPECT_EQ(tree("sin x cos x"), "sin(x) * cos(x)");
    EXPECT_EQ(tree("sin -x"), "sin(-x)");
    EXPECT_EQ(tree("sin cos x"), "sin(cos(x))");
    EXPECT_EQ(tree("2sin x"), "2 * sin(x)");
}

TEST(Parser, FunctionPower) {
    EXPECT_EQ(tree("sin^2 x"), "sin(x)^2");
    EXPECT_EQ(tree("sin^2(x+1)"), "sin(x + 1)^2");
    EXPECT_EQ(tree("sin^2 x^3"), "sin(x^3)^2");
    EXPECT_NEAR(value("sin^2 x + cos^2 x", {{"x", 0.7}}), 1.0, EPS);
}

/// CONSTANTS ///

TEST(Parser, Constants) {
    EXPECT_NEAR(value("pi"), std::acos(-1.0), EPS);
    EXPECT_NEAR(value("e"), std::exp(1.0), EPS);
    EXPECT_NEAR(value("sin(pi / 2)"), 1.0, EPS);
    EXPECT_EQ(parse("2 * pi")->simplify()->toString(), "2 * pi");    // not folded into 6.28319
}

/// END TO END: PARSE -> DIFFERENTIATE -> SIMPLIFY ///

TEST(Parser, Derivatives) {
    EXPECT_EQ(derivative("x^2 + 3x"), "2 * x + 3");
    EXPECT_EQ(derivative("sin x^2"), "2 * cos(x^2) * x");
    EXPECT_EQ(derivative("e^x"), "e^x");
    EXPECT_EQ(derivative("ln x"), "1 / x");
    EXPECT_EQ(derivative("pi x"), "pi");
    EXPECT_EQ(derivative("1/2x"), "1 / 2");
}

/// ERRORS ///

TEST(ParserErrors, EmptyAndIncompleteInput) {
    EXPECT_EQ(errorMessage(""), "Unexpected end of input at column 1");
    EXPECT_EQ(errorPosition("()"), 1);
    EXPECT_EQ(errorPosition("x +"), 3);
    EXPECT_EQ(errorPosition("* x"), 0);
}

TEST(ParserErrors, Parentheses) {
    EXPECT_EQ(errorMessage("(x + 1"), "Expected ')' but found end of input at column 7");
    EXPECT_EQ(errorMessage("x + 1)"), "Unexpected ')' at column 6");
}

TEST(ParserErrors, NumberAfterOperand) {
    EXPECT_EQ(errorPosition("2 3"), 2);
    EXPECT_EQ(errorPosition("x2"), 1);
}

TEST(ParserErrors, Functions) {
    EXPECT_EQ(errorPosition("sin"), 3);
    EXPECT_EQ(errorPosition("sin * x"), 4);
    EXPECT_EQ(tree("sin + x"), "sin(+x)");    // like sin -x = sin(-x)
    EXPECT_EQ(errorPosition("sin^-1 x"), 4);
    EXPECT_EQ(errorPosition("sin^x x"), 4);
    EXPECT_EQ(errorPosition("sin^0 x"), 4);
}

TEST(ParserErrors, ParseErrorIsMateckError) {
    EXPECT_THROW(parse("(x"), MateckError);
    EXPECT_THROW(parse("x $"), LexError);    // lexer errors pass through parse()
}
