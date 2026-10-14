#include "test.h"
#include "node.h"
#include "errors.h"

#include <cmath>

void runNodeTests() {

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

    // Hibatipusok
    {
        Environment empty;
        checkBool("ismeretlen valtozo -> EvalError",
            throwsError<EvalError>([&] { VarNode("z").evaluate(empty); }), true);

        checkBool("floor derivalasa -> DiffError",
            throwsError<DiffError>([] { FuncNode(FunctionType::FLOOR, std::make_unique<VarNode>("x")).differentiate("x"); }), true);
    }

    // Fuggvenynevek oda-vissza
    checkStr("functionTypeToString(ARCSIN)", functionTypeToString(FunctionType::ARCSIN), "arcsin");
    checkBool("stringToFunctionType(\"acoth\")", stringToFunctionType("acoth") == FunctionType::ACOTH, true);
    checkBool("stringToFunctionType(\"foo\") ures", stringToFunctionType("foo").has_value(), false);
    check("constantValue(\"pi\")", *constantValue("pi"), std::acos(-1.0));
}
