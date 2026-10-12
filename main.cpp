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

    // toString: zarojelezes a precedencia szerint
    {
        auto x = [] { return std::make_unique<VarNode>("x"); };
        auto num = [](double v) { return std::make_unique<NumberNode>(v); };

        BinaryOpNode sum(BinaryOperator::MUL,
            std::make_unique<BinaryOpNode>(BinaryOperator::ADD, x(), num(2)),
            std::make_unique<BinaryOpNode>(BinaryOperator::SUB, x(), num(1)));
        checkStr("toString (x+2)*(x-1)", sum.toString(), "(x + 2) * (x - 1)");

        BinaryOpNode subRight(BinaryOperator::SUB, x(),
            std::make_unique<BinaryOpNode>(BinaryOperator::SUB, x(), num(1)));
        checkStr("toString x-(x-1)", subRight.toString(), "x - (x - 1)");

        BinaryOpNode powLeft(BinaryOperator::POW,
            std::make_unique<BinaryOpNode>(BinaryOperator::POW, x(), num(2)), num(3));
        checkStr("toString (x^2)^3", powLeft.toString(), "(x^2)^3");

        BinaryOpNode powNeg(BinaryOperator::POW, num(-1), num(2));
        checkStr("toString (-1)^2", powNeg.toString(), "(-1)^2");

        UnaryOpNode negPow(UnaryOperator::SUB,
            std::make_unique<BinaryOpNode>(BinaryOperator::POW, x(), num(2)));
        checkStr("toString -x^2", negPow.toString(), "-x^2");

        FuncNode sinSum(FunctionType::SIN,
            std::make_unique<BinaryOpNode>(BinaryOperator::ADD, x(), num(1)));
        checkStr("toString sin(x + 1)", sinSum.toString(), "sin(x + 1)");
    }

    // simplify: azonossagok es konstans osszevonas
    {
        auto x = [] { return std::make_unique<VarNode>("x"); };
        auto num = [](double v) { return std::make_unique<NumberNode>(v); };
        auto bin = [](BinaryOperator op, std::unique_ptr<Node> l, std::unique_ptr<Node> r) {
            return std::make_unique<BinaryOpNode>(op, std::move(l), std::move(r));
        };

        checkStr("simplify 0 + x", bin(BinaryOperator::ADD, num(0), x())->simplify()->toString(), "x");
        checkStr("simplify x - 0", bin(BinaryOperator::SUB, x(), num(0))->simplify()->toString(), "x");
        checkStr("simplify 0 - x", bin(BinaryOperator::SUB, num(0), x())->simplify()->toString(), "-x");
        checkStr("simplify 1 * x", bin(BinaryOperator::MUL, num(1), x())->simplify()->toString(), "x");
        checkStr("simplify x * 0", bin(BinaryOperator::MUL, x(), num(0))->simplify()->toString(), "0");
        checkStr("simplify -1 * x", bin(BinaryOperator::MUL, num(-1), x())->simplify()->toString(), "-x");
        checkStr("simplify 0 / x", bin(BinaryOperator::DIV, num(0), x())->simplify()->toString(), "0");
        checkStr("simplify x / 1", bin(BinaryOperator::DIV, x(), num(1))->simplify()->toString(), "x");
        checkStr("simplify x^0", bin(BinaryOperator::POW, x(), num(0))->simplify()->toString(), "1");
        checkStr("simplify x^1", bin(BinaryOperator::POW, x(), num(1))->simplify()->toString(), "x");
        checkStr("simplify 1^x", bin(BinaryOperator::POW, num(1), x())->simplify()->toString(), "1");
        checkStr("simplify 2 * 3 + x", bin(BinaryOperator::ADD,
            bin(BinaryOperator::MUL, num(2), num(3)), x())->simplify()->toString(), "x + 6");
        checkStr("simplify 1 / 0 marad", bin(BinaryOperator::DIV, num(1), num(0))->simplify()->toString(), "1 / 0");
        checkStr("simplify 0 / 0 marad", bin(BinaryOperator::DIV, num(0), num(0))->simplify()->toString(), "0 / 0");

        UnaryOpNode doubleNeg(UnaryOperator::SUB,
            std::make_unique<UnaryOpNode>(UnaryOperator::SUB, x()));
        checkStr("simplify -(-x)", doubleNeg.simplify()->toString(), "x");

        FuncNode sinZero(FunctionType::SIN, num(0));
        checkStr("simplify sin(0)", sinZero.simplify()->toString(), "0");

        FuncNode lnZero(FunctionType::LN, num(0));
        checkStr("simplify ln(0) marad", lnZero.simplify()->toString(), "ln(0)");

        // az eredeti fa nem valtozik
        auto original = bin(BinaryOperator::MUL, num(1), x());
        original->simplify();
        checkStr("simplify nem modositja az eredetit", original->toString(), "1 * x");
    }

    // simplify a derivaltakon
    {
        auto x = [] { return std::make_unique<VarNode>("x"); };
        auto num = [](double v) { return std::make_unique<NumberNode>(v); };

        BinaryOpNode square(BinaryOperator::POW, x(), num(2));
        checkStr("d/dx x^2 egyszerusitve", square.differentiate("x")->simplify()->toString(), "2 * x");

        BinaryOpNode cube(BinaryOperator::POW, x(), num(3));
        checkStr("d/dx x^3 egyszerusitve", cube.differentiate("x")->simplify()->toString(), "3 * x^2");

        FuncNode sinX(FunctionType::SIN, x());
        checkStr("d/dx sin(x) egyszerusitve", sinX.differentiate("x")->simplify()->toString(), "cos(x)");

        FuncNode cosX(FunctionType::COS, x());
        checkStr("d/dx cos(x) egyszerusitve", cosX.differentiate("x")->simplify()->toString(), "-sin(x)");

        // korabban NaN volt x=0-nal: 0 * x^(-1) * 1
        BinaryOpNode zeroPow(BinaryOperator::POW, x(), num(0));
        Environment env0;
        env0["x"] = 0.0;
        check("d/dx x^0 egyszerusitve, x=0", zeroPow.differentiate("x")->simplify()->evaluate(env0), 0.0);
    }

    // equals / operator==: szerkezeti egyenloseg
    {
        auto x = [] { return std::make_unique<VarNode>("x"); };
        auto num = [](double v) { return std::make_unique<NumberNode>(v); };
        auto bin = [](BinaryOperator op, std::unique_ptr<Node> l, std::unique_ptr<Node> r) {
            return std::make_unique<BinaryOpNode>(op, std::move(l), std::move(r));
        };

        checkBool("x + 1 == x + 1", *bin(BinaryOperator::ADD, x(), num(1)) == *bin(BinaryOperator::ADD, x(), num(1)), true);
        checkBool("x + 1 != 1 + x (szerkezetileg)", *bin(BinaryOperator::ADD, x(), num(1)) != *bin(BinaryOperator::ADD, num(1), x()), true);
        checkBool("x + 1 != x - 1", *bin(BinaryOperator::ADD, x(), num(1)) == *bin(BinaryOperator::SUB, x(), num(1)), false);
        checkBool("x != y", VarNode("x") == VarNode("y"), false);
        checkBool("2 == 2", NumberNode(2) == NumberNode(2), true);
        checkBool("sin(x) != cos(x)", FuncNode(FunctionType::SIN, x()) == FuncNode(FunctionType::COS, x()), false);
        checkBool("2 != x", NumberNode(2) == VarNode("x"), false);
    }

    // simplify: lapitas, osszevonas, kanonikus sorrend
    {
        auto x = [] { return std::make_unique<VarNode>("x"); };
        auto y = [] { return std::make_unique<VarNode>("y"); };
        auto num = [](double v) { return std::make_unique<NumberNode>(v); };
        auto bin = [](BinaryOperator op, std::unique_ptr<Node> l, std::unique_ptr<Node> r) {
            return std::make_unique<BinaryOpNode>(op, std::move(l), std::move(r));
        };
        auto simp = [](const std::unique_ptr<Node>& n) { return n->simplify()->toString(); };
        using B = BinaryOperator;

        // a megbeszelt negy pelda
        checkStr("simplify x + 1 - x", simp(bin(B::SUB, bin(B::ADD, x(), num(1)), x())), "1");
        checkStr("simplify (x + 1) * (x + 1)", simp(bin(B::MUL, bin(B::ADD, x(), num(1)), bin(B::ADD, x(), num(1)))), "(x + 1)^2");
        checkStr("simplify 3 / (3 * x)", simp(bin(B::DIV, num(3), bin(B::MUL, num(3), x()))), "1 / x");
        checkStr("simplify 2 * (3 * x)", simp(bin(B::MUL, num(2), bin(B::MUL, num(3), x()))), "6 * x");

        // osszevonas
        checkStr("simplify x + x", simp(bin(B::ADD, x(), x())), "2 * x");
        checkStr("simplify x - x", simp(bin(B::SUB, x(), x())), "0");
        checkStr("simplify 2 * x + 3 * x", simp(bin(B::ADD, bin(B::MUL, num(2), x()), bin(B::MUL, num(3), x()))), "5 * x");
        checkStr("simplify x * y - y * x", simp(bin(B::SUB, bin(B::MUL, x(), y()), bin(B::MUL, y(), x()))), "0");
        checkStr("simplify x * x^2", simp(bin(B::MUL, x(), bin(B::POW, x(), num(2)))), "x^3");
        checkStr("simplify x / x", simp(bin(B::DIV, x(), x())), "1");
        checkStr("simplify x^2 / x", simp(bin(B::DIV, bin(B::POW, x(), num(2)), x())), "x");
        checkStr("simplify x / 3 + x / 3", simp(bin(B::ADD, bin(B::DIV, x(), num(3)), bin(B::DIV, x(), num(3)))), "2 * x / 3");
        checkStr("simplify 6 * x / 4", simp(bin(B::DIV, bin(B::MUL, num(6), x()), num(4))), "3 * x / 2");

        // kanonikus sorrend es elojelek
        checkStr("simplify 1 + x", simp(bin(B::ADD, num(1), x())), "x + 1");
        checkStr("simplify y + x", simp(bin(B::ADD, y(), x())), "x + y");
        checkStr("simplify y * x", simp(bin(B::MUL, y(), x())), "x * y");
        checkStr("simplify 1 - x", simp(bin(B::SUB, num(1), x())), "-x + 1");
        checkStr("simplify -(x + 1)", simp(std::make_unique<UnaryOpNode>(UnaryOperator::SUB, bin(B::ADD, x(), num(1)))), "-x - 1");
        checkStr("simplify -(2 * x)", simp(std::make_unique<UnaryOpNode>(UnaryOperator::SUB, bin(B::MUL, num(2), x()))), "-2 * x");
        checkStr("simplify x * -y", simp(bin(B::MUL, x(), std::make_unique<UnaryOpNode>(UnaryOperator::SUB, y()))), "-(x * y)");

        // ket egyenlo kifejezes kanonikus alakja szerkezetileg is egyezik
        auto a = bin(B::ADD, bin(B::MUL, num(2), x()), bin(B::ADD, y(), num(1)));    // 2 * x + (y + 1)
        auto b = bin(B::ADD, num(1), bin(B::ADD, y(), bin(B::MUL, x(), num(2))));    // 1 + (y + x * 2)
        checkBool("2x + (y + 1) es 1 + (y + x*2) kanonikus alakja egyezik", *a->simplify() == *b->simplify(), true);

        // idempotencia: simplify(simplify(f)) == simplify(f)
        auto messy = bin(B::SUB,
            bin(B::DIV, bin(B::MUL, num(-2), bin(B::POW, x(), num(3))), bin(B::MUL, num(4), y())),
            bin(B::ADD, std::make_unique<UnaryOpNode>(UnaryOperator::SUB, x()), bin(B::DIV, num(1), y())));
        auto once = messy->simplify();
        checkBool("simplify idempotens", *once->simplify() == *once, true);
        check("simplify nem valtoztatja az erteket", once->evaluate({{"x", 1.7}, {"y", -0.4}}),
                                                      messy->evaluate({{"x", 1.7}, {"y", -0.4}}));
    }

    std::cout << "\n" << passed << "/" << total << " teszt sikeres.\n";

    return (passed == total) ? 0 : 1;
}