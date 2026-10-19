#include <gtest/gtest.h>

#include "repl.h"

#include <iomanip>
#include <sstream>

namespace {

    // runs the interactive mode on the given input and returns everything it printed
    std::string run(const std::string& input) {
        std::istringstream in(input);
        std::ostringstream out;
        runInteractive(in, out);
        return out.str();
    }

    // one result line, formatted like the REPL does
    std::string line(const std::string& label, const std::string& text) {
        std::ostringstream out;
        out << "  " << std::left << std::setw(22) << label << text << "\n";
        return out.str();
    }

    bool contains(const std::string& text, const std::string& part) {
        return text.find(part) != std::string::npos;
    }

}

TEST(Repl, ShowsParsedSimplifiedAndDerivative) {
    std::string out = run("x^3 - 2x\n\n");
    EXPECT_TRUE(contains(out, line("ertelmezes:", "x^3 - 2x"))) << out;
    EXPECT_TRUE(contains(out, line("egyszerusitve:", "x^3 - 2x"))) << out;
    EXPECT_TRUE(contains(out, line("d/dx:", "3x^2 - 2"))) << out;
}

TEST(Repl, ShowsExpandedForm) {
    std::string out = run("(x + 1)(x - 1)\n\n");
    EXPECT_TRUE(contains(out, line("egyszerusitve:", "(x + 1)(x - 1)"))) << out;
    EXPECT_TRUE(contains(out, line("kibontva:", "x^2 - 1"))) << out;
}

TEST(Repl, ExpandedFormTooLong) {
    std::string out = run("(x + 1)^60\n\n");
    EXPECT_TRUE(contains(out, line("kibontva:", "(tul hosszu)"))) << out;
}

TEST(Repl, AllVariablesGiven) {
    std::string out = run("sin(x + y)\npi/2\n0\n");
    EXPECT_TRUE(contains(out, line("ertek:", "1"))) << out;
    EXPECT_TRUE(contains(out, line("d/dx ertek:", "0"))) << out;    // cos(pi/2) is 6.1e-17, shown as 0
}

TEST(Repl, SomeVariablesGiven) {
    std::string out = run("sin(x + y)\n2\n\n");
    EXPECT_TRUE(contains(out, line("behelyettesitve:", "sin(y + 2)"))) << out;
    EXPECT_TRUE(contains(out, line("d/dx behelyettesitve:", "cos(y + 2)"))) << out;
    EXPECT_FALSE(contains(out, "ertek:")) << out;
}

TEST(Repl, NoVariablesGiven) {
    std::string out = run("sin(x + y)\n\n\n");
    EXPECT_FALSE(contains(out, "ertek:")) << out;
    EXPECT_FALSE(contains(out, "behelyettesitve:")) << out;
}

TEST(Repl, ExpressionWithoutVariables) {
    std::string out = run("2 + 3\n");
    EXPECT_FALSE(contains(out, " = ")) << out;    // nothing to ask
    EXPECT_TRUE(contains(out, line("ertek:", "5"))) << out;
}

TEST(Repl, AsksInOrderOfFirstAppearance) {
    std::string out = run("y + x\n1\n2\n");
    EXPECT_LT(out.find("y = "), out.find("x = ")) << out;
    EXPECT_TRUE(contains(out, line("ertek:", "3"))) << out;
}

TEST(Repl, AsksAgainAfterValueWithVariable) {
    std::string out = run("x^2\n2y\n3\n");
    EXPECT_TRUE(contains(out, "Hiba: az ertek nem tartalmazhat valtozot (y)")) << out;
    EXPECT_TRUE(contains(out, line("ertek:", "9"))) << out;
    EXPECT_TRUE(contains(out, line("d/dx ertek:", "6"))) << out;
}

TEST(Repl, AsksAgainAfterInvalidValue) {
    std::string out = run("x^2\n(1\n3\n");
    EXPECT_TRUE(contains(out, "Hiba: Expected ')'")) << out;
    EXPECT_TRUE(contains(out, line("ertek:", "9"))) << out;
}

TEST(Repl, UndefinedValue) {
    std::string out = run("sqrt x\n-1\n");
    EXPECT_TRUE(contains(out, line("ertek:", "nincs ertelmezve"))) << out;
}

TEST(Repl, NonDifferentiableFunction) {
    std::string out = run("floor x\n2.5\n");
    EXPECT_TRUE(contains(out, line("d/dx:", "Cannot differentiate a discontinuous function (floor/ceil/round)"))) << out;
    EXPECT_TRUE(contains(out, line("ertek:", "2"))) << out;
    EXPECT_FALSE(contains(out, "d/dx ertek:")) << out;
}

TEST(Repl, ParseErrorContinuesWithNextLine) {
    std::string out = run("sin(\n1 + 1\n");
    EXPECT_TRUE(contains(out, "Hiba: ")) << out;
    EXPECT_TRUE(contains(out, line("ertek:", "2"))) << out;
}

TEST(Repl, InputEndsWhileAskingForValue) {
    std::string out = run("x^2\n");
    EXPECT_TRUE(contains(out, "  x = \n")) << out;
    EXPECT_FALSE(contains(out, "ertek:")) << out;
}
