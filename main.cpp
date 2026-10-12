#include "node.h"
#include "lexer.h"

#include <iostream>
#include <cmath>

int passed = 0;
int total = 0;

void check(const std::string& label, double actual, double expected, double eps = 1e-9) {
    total++;
    bool ok = std::fabs(actual - expected) < eps;
    if (ok) passed++;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label
               << " (elvart: " << expected << ", kapott: " << actual << ")\n";
}

void checkBool(const std::string& label, bool actual, bool expected) {
    total++;
    bool ok = (actual == expected);
    if (ok) passed++;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label
               << " (elvart: " << expected << ", kapott: " << actual << ")\n";
}

void checkStr(const std::string& label, const std::string& actual, const std::string& expected) {
    total++;
    bool ok = (actual == expected);
    if (ok) passed++;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label
               << " (elvart: " << expected << ", kapott: " << actual << ")\n";
}

// tokenek tomor szoveges alakja, pl. "F(sin) V(x)"
std::string lex(const std::string& input) {
    std::string result;
    try {
        Lexer lexer(input);
        for (const Token& t : lexer.tokenize()) {
            if (!result.empty()) result += " ";
            result += tokenTypeToString(t.getType()).substr(0, 1) + "(" + t.getValue() + ")";
        }
    }
    catch (const std::exception&) {
        return "HIBA";
    }
    return result;
}

int main() {
    Environment env;
    env["x"] = 3.0;

    // NumberNode alap
    {
        NumberNode n(5.0);
        check("NumberNode evaluate", n.evaluate(env), 5.0);
        checkBool("NumberNode isConstant", n.isConstant("x"), true);
        auto d = n.differentiate("x");
        check("NumberNode differentiate", d->evaluate(env), 0.0);
    }

    // VarNode alap
    {
        VarNode v("x");
        check("VarNode evaluate", v.evaluate(env), 3.0);
        checkBool("VarNode isConstant sajat valtozora", v.isConstant("x"), false);
        checkBool("VarNode isConstant mas valtozora", v.isConstant("y"), true);
        auto d = v.differentiate("x");
        check("VarNode differentiate sajat valtozo szerint", d->evaluate(env), 1.0);
    }

    // BinaryOpNode: (x + 2) * (x - 1), x = 3 -> (5)*(2) = 10
    {
        auto left = std::make_unique<BinaryOpNode>(
            BinaryOperator::ADD, std::make_unique<VarNode>("x"), std::make_unique<NumberNode>(2.0));
        auto right = std::make_unique<BinaryOpNode>(
            BinaryOperator::SUB, std::make_unique<VarNode>("x"), std::make_unique<NumberNode>(1.0));
        BinaryOpNode expr(BinaryOperator::MUL, std::move(left), std::move(right));

        check("(x+2)*(x-1) evaluate x=3", expr.evaluate(env), 10.0);
    }

    // Hatvanyszabaly: x^2 differentialva, x=5 -> 2x = 10
    {
        BinaryOpNode square(BinaryOperator::POW,
            std::make_unique<VarNode>("x"), std::make_unique<NumberNode>(2.0));

        Environment env5;
        env5["x"] = 5.0;

        auto derivative = square.differentiate("x");
        check("x^2 derivalt evaluate x=5", derivative->evaluate(env5), 10.0);
    }

    // Szorzatszabaly: x * x differentialva, x=5 -> 2x = 10
    {
        BinaryOpNode product(BinaryOperator::MUL,
            std::make_unique<VarNode>("x"), std::make_unique<VarNode>("x"));

        Environment env5;
        env5["x"] = 5.0;

        auto derivative = product.differentiate("x");
        check("x*x derivalt evaluate x=5 (szorzatszabaly)", derivative->evaluate(env5), 10.0);
    }

    // Hanyadosszabaly: x / (x+1) differentialva, x=2 -> 1/(x+1)^2 = 1/9
    {
        auto num = std::make_unique<VarNode>("x");
        auto denom = std::make_unique<BinaryOpNode>(
            BinaryOperator::ADD, std::make_unique<VarNode>("x"), std::make_unique<NumberNode>(1.0));
        BinaryOpNode quotient(BinaryOperator::DIV, std::move(num), std::move(denom));

        Environment env2;
        env2["x"] = 2.0;

        auto derivative = quotient.differentiate("x");
        check("x/(x+1) derivalt evaluate x=2 (hanyadosszabaly)", derivative->evaluate(env2), 1.0 / 9.0);
    }

    // clone teszt: klonozott fa ugyanazt adja vissza, mint az eredeti
    {
        BinaryOpNode original(BinaryOperator::ADD,
            std::make_unique<VarNode>("x"), std::make_unique<NumberNode>(1.0));
        auto cloned = original.clone();

        check("clone() ugyanazt az eredmenyt adja", cloned->evaluate(env), original.evaluate(env));
    }

    // Lexer: ideiglenes stringbol is biztonsagosan dolgozik (korabban logo referencia)
    {
        Lexer lexer(std::string("x+1"));
        checkStr("Lexer ideiglenes stringgel", std::to_string(lexer.tokenize().size()), "3");
    }

    // Lexer: fuggvenynev utan kozvetlenul irt valtozo
    checkStr("Lexer sinx", lex("sinx"), "F(sin) V(x)");
    checkStr("Lexer sinhx (leghosszabb egyezes)", lex("sinhx"), "F(sinh) V(x)");
    checkStr("Lexer xsin(y)", lex("xsin(y)"), "V(x) F(sin) L(() V(y) R())");
    checkStr("Lexer 2xy", lex("2xy"), "N(2) V(x) V(y)");

    // Lexer: szamok
    checkStr("Lexer .5", lex(".5"), "N(.5)");
    checkStr("Lexer 1.2.3 hiba", lex("1.2.3"), "HIBA");
    checkStr("Lexer nem ASCII karakter hiba", lex("2*\xCF\x80"), "HIBA");

    // Konstans reszfa derivaltja 0, nem NaN
    {
        Environment envY;
        envY["x"] = 2.0;
        envY["y"] = 0.0;

        FuncNode sqrtY(FunctionType::SQRT, std::make_unique<VarNode>("y"));
        check("d/dx sqrt(y), y=0", sqrtY.differentiate("x")->evaluate(envY), 0.0);

        FuncNode lnY(FunctionType::LN, std::make_unique<VarNode>("y"));
        check("d/dx ln(y), y=0", lnY.differentiate("x")->evaluate(envY), 0.0);

        BinaryOpNode zeroPowY(BinaryOperator::POW,
            std::make_unique<NumberNode>(0.0), std::make_unique<VarNode>("y"));
        check("d/dx 0^y", zeroPowY.differentiate("x")->evaluate(envY), 0.0);

        // x * sqrt(y) -> sqrt(y) = 0
        BinaryOpNode xTimesSqrtY(BinaryOperator::MUL,
            std::make_unique<VarNode>("x"),
            std::make_unique<FuncNode>(FunctionType::SQRT, std::make_unique<VarNode>("y")));
        check("d/dx x*sqrt(y), y=0", xTimesSqrtY.differentiate("x")->evaluate(envY), 0.0);
    }

    // arccot ertekkeszlete (0, pi)
    {
        FuncNode acotNeg(FunctionType::ARCCOT, std::make_unique<NumberNode>(-1.0));
        check("arccot(-1) = 3pi/4", acotNeg.evaluate(env), 3.0 * std::atan(1.0));

        FuncNode acotZero(FunctionType::ARCCOT, std::make_unique<NumberNode>(0.0));
        check("arccot(0) = pi/2", acotZero.evaluate(env), 2.0 * std::atan(1.0));
    }

    std::cout << "\n" << passed << "/" << total << " teszt sikeres.\n";

    return (passed == total) ? 0 : 1;
}