#include "node.h"

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

    std::cout << "\n" << passed << "/" << total << " teszt sikeres.\n";

    return (passed == total) ? 0 : 1;
}