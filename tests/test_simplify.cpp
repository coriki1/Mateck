#include "test.h"
#include "node.h"

#include <cmath>

void runSimplifyTests() {

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
}
