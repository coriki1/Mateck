#include "format.h"

#include <cctype>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace {

    constexpr int SIGNIFICANT_DIGITS = 10;

    // "1.2300" -> "1.23", "5.000" -> "5"
    std::string trimZeros(std::string s) {
        if (s.find('.') == std::string::npos) {
            return s;
        }
        while (s.back() == '0') {
            s.pop_back();
        }
        if (s.back() == '.') {
            s.pop_back();
        }
        return s;
    }

    int formatPrecedence(const Node& n);

    // whether the argument of a unary sign needs parentheses: -(x + 1) and -(-x) do, -2x and -x^2 do not
    bool unaryParentheses(const Node& arg, const std::string& argText) {
        return formatPrecedence(arg) < Precedence::MULTIPLICATIVE || argText[0] == '-' || argText[0] == '+';
    }

    // how strongly the formatted text binds, which can differ from Node::precedence():
    // -(x y) is written as -x y, so it binds like a product, not like a unary minus
    int formatPrecedence(const Node& n) {
        auto* u = dynamic_cast<const UnaryOpNode*>(&n);
        if (!u) {
            return n.precedence();
        }
        if (unaryParentheses(u->getArg(), format(u->getArg()))) {
            return Precedence::UNARY;
        }
        return std::min(Precedence::UNARY, formatPrecedence(u->getArg()));
    }

    std::string formatChild(const Node& child, bool parentheses) {
        std::string text = format(child);
        return parentheses ? "(" + text + ")" : text;
    }

    // whether text can follow another operand without '*': the parser reads it as implicit multiplication
    bool startsWithNameOrParen(const std::string& text) {
        return !text.empty() && (std::isalpha(static_cast<unsigned char>(text[0])) || text[0] == '(');
    }

    std::string formatProduct(const BinaryOpNode& n) {

        const Node& left = n.getLeft();
        const Node& right = n.getRight();

        std::string l = formatChild(left, formatPrecedence(left) < Precedence::MULTIPLICATIVE);
        std::string r = formatChild(right, formatPrecedence(right) < Precedence::MULTIPLICATIVE);

        if (!startsWithNameOrParen(r)) { // x * 3, x * -y: "x 3" or "x -y" would read differently
            return l + " * " + r;
        }

        // 2x, 2sin(x), 2(x + 1), (x + 1)(x - 1)
        if (left.numericValue() || (l.back() == ')' && r[0] == '(')) {
            return l + r;
        }

        // x y, x sin(x), pi x: without the space "p i" would become "pi" and "a sinh" would become "asinh"
        return l + " " + r;
    }

    std::string formatQuotient(const BinaryOpNode& n) {

        const Node& left = n.getLeft();
        const Node& right = n.getRight();

        // a/(2b): an implicit product binds like *, so a/2b would mean (a/2) b
        std::string l = formatChild(left, formatPrecedence(left) < Precedence::MULTIPLICATIVE);
        std::string r = formatChild(right, formatPrecedence(right) <= Precedence::MULTIPLICATIVE);

        return l + "/" + r;
    }

    std::string formatBinary(const BinaryOpNode& n) {

        const Node& left = n.getLeft();
        const Node& right = n.getRight();
        int p = n.precedence();

        switch (n.getOp()) {

            case BinaryOperator::MUL:
                return formatProduct(n);

            case BinaryOperator::DIV:
                return formatQuotient(n);

            case BinaryOperator::POW: // same parentheses as toString(): ^ is right-associative
                return formatChild(left, formatPrecedence(left) <= p) + "^"
                     + formatChild(right, formatPrecedence(right) < p);

            case BinaryOperator::ADD:
            case BinaryOperator::SUB: {
                std::string symbol = n.getOp() == BinaryOperator::ADD ? " + " : " - ";
                bool rightParentheses = formatPrecedence(right) < p
                                     || (formatPrecedence(right) == p && n.getOp() == BinaryOperator::SUB);
                return formatChild(left, formatPrecedence(left) < p) + symbol + formatChild(right, rightParentheses);
            }
        }

        return n.toString();
    }

    std::string formatUnary(const UnaryOpNode& n) {

        std::string sign = n.getOp() == UnaryOperator::SUB ? "-" : "+";
        std::string arg = format(n.getArg());

        bool parentheses = unaryParentheses(n.getArg(), arg);

        return sign + (parentheses ? "(" + arg + ")" : arg);
    }

}

std::string formatNumber(double v) {

    if (v == 0.0) { // also turns -0 into 0
        return "0";
    }

    std::ostringstream out;
    out << std::setprecision(SIGNIFICANT_DIGITS) << v;
    std::string text = out.str();

    if (text.find('e') == std::string::npos) {
        return text;
    }

    // the parser has no scientific notation: write 1e+20 and 1e-12 out in full
    int exponent = static_cast<int>(std::floor(std::log10(std::abs(v))));
    int decimals = std::max(0, SIGNIFICANT_DIGITS - 1 - exponent);

    std::ostringstream fixed;
    fixed << std::fixed << std::setprecision(decimals) << v;
    return trimZeros(fixed.str());
}

std::string format(const Node& node) {

    if (auto v = node.numericValue()) {
        return formatNumber(*v);
    }
    if (auto* b = dynamic_cast<const BinaryOpNode*>(&node)) {
        return formatBinary(*b);
    }
    if (auto* u = dynamic_cast<const UnaryOpNode*>(&node)) {
        return formatUnary(*u);
    }
    if (auto* f = dynamic_cast<const FuncNode*>(&node)) {
        return functionTypeToString(f->getFunc()) + "(" + format(f->getArg()) + ")";
    }

    return node.toString(); // variables and constants: their name
}
