#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "simplify.h"

// Algebraic identities are applied without checking the domain, like most CAS do:
// 0 * ln(x) becomes 0 and x / x becomes 1, even though they are undefined for some x.

namespace {

    constexpr int MAX_PASSES = 20; // safety net in case two forms keep turning into each other

    /// FLAT FORMS ///////////////////////////////////////////////////////////////////////

    // numerator / denominator * f1^e1 * f2^e2 * ...
    struct FlatProduct {
        double numerator = 1.0;
        double denominator = 1.0;
        std::vector<std::pair<std::unique_ptr<Node>, double>> factors;
    };

    // one term of a sum: numerator / denominator * key, e.g. 2/3 * x^2
    struct Term {
        double numerator;
        double denominator;
        std::unique_ptr<Node> key;
    };

    // t1 + t2 + ... + constant
    struct FlatSum {
        double constant = 0.0;
        std::vector<Term> terms;
    };

    /// HELPERS //////////////////////////////////////////////////////////////////////////

    // canonical order of terms and factors, so that x + 1 and 1 + x end up the same
    bool canonicalLess(const Node& a, const Node& b) {
        return a.toString() < b.toString();
    }

    bool isInteger(double v) {
        return std::abs(v) < 1e15 && v == std::round(v);
    }

    // 6/4 -> 3/2, keeps the fraction exact while both parts are integers
    void reduceFraction(double& numerator, double& denominator) {

        if (isInteger(numerator) && isInteger(denominator)) {
            long long g = std::gcd(std::llround(numerator), std::llround(denominator));
            if (g > 1) {
                numerator /= g;
                denominator /= g;
            }
        }
        else {
            numerator /= denominator;
            denominator = 1.0;
        }

        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
    }

    std::unique_ptr<Node> multiply(std::unique_ptr<Node> acc, std::unique_ptr<Node> factor) {
        if (!acc) {
            return factor;
        }
        return std::make_unique<BinaryOpNode>(BinaryOperator::MUL, std::move(acc), std::move(factor));
    }

    /// PRODUCT //////////////////////////////////////////////////////////////////////////

    // 3 / (3 * x)  ->  numerator 3, denominator 3, factors [(x, -1)]
    void flattenProduct(const Node& node, bool inDenominator, FlatProduct& out) {

        if (auto v = node.numericValue()) {
            (inDenominator ? out.denominator : out.numerator) *= *v;
            return;
        }

        if (auto* bin = dynamic_cast<const BinaryOpNode*>(&node)) {
            switch (bin->getOp()) {
                case BinaryOperator::MUL:
                    flattenProduct(bin->getLeft(), inDenominator, out);
                    flattenProduct(bin->getRight(), inDenominator, out);
                    return;

                case BinaryOperator::DIV: // division flips the side of the right operand
                    flattenProduct(bin->getLeft(), inDenominator, out);
                    flattenProduct(bin->getRight(), !inDenominator, out);
                    return;

                case BinaryOperator::POW: // x^2 -> factor x with exponent 2
                    if (auto k = bin->getRight().numericValue()) {
                        out.factors.emplace_back(bin->getLeft().clone(), inDenominator ? -*k : *k);
                        return;
                    }
                    break;

                default:
                    break;
            }
        }

        if (auto* un = dynamic_cast<const UnaryOpNode*>(&node)) {
            if (un->getOp() == UnaryOperator::SUB) {
                out.numerator = -out.numerator;
            }
            flattenProduct(un->getArg(), inDenominator, out);
            return;
        }

        out.factors.emplace_back(node.clone(), inDenominator ? -1.0 : 1.0);
    }

    // x * x^2 / x -> x^2
    void collectFactors(FlatProduct& p) {

        std::vector<std::pair<std::unique_ptr<Node>, double>> merged;

        for (auto& [factor, exponent] : p.factors) {
            auto same = std::find_if(merged.begin(), merged.end(),
                [&](const auto& m) { return *m.first == *factor; });

            if (same != merged.end()) {
                same->second += exponent;
            }
            else {
                merged.emplace_back(std::move(factor), exponent);
            }
        }

        merged.erase(std::remove_if(merged.begin(), merged.end(),
            [](const auto& m) { return m.second == 0.0; }), merged.end());

        std::sort(merged.begin(), merged.end(),
            [](const auto& a, const auto& b) { return canonicalLess(*a.first, *b.first); });

        p.factors = std::move(merged);
    }

    // numerator * positive factors / (denominator * negative factors)
    std::unique_ptr<Node> buildProduct(const FlatProduct& p) {

        std::unique_ptr<Node> top;
        std::unique_ptr<Node> bottom;

        if (p.numerator != 1.0 && p.numerator != -1.0) {
            top = std::make_unique<NumberNode>(p.numerator);
        }
        if (p.denominator != 1.0) {
            bottom = std::make_unique<NumberNode>(p.denominator);
        }

        for (const auto& [factor, exponent] : p.factors) {

            double k = std::abs(exponent);
            std::unique_ptr<Node> power = (k == 1.0)
                ? factor->clone()
                : std::make_unique<BinaryOpNode>(BinaryOperator::POW, factor->clone(), std::make_unique<NumberNode>(k));

            if (exponent > 0) {
                top = multiply(std::move(top), std::move(power));
            }
            else {
                bottom = multiply(std::move(bottom), std::move(power));
            }
        }

        if (!top) {
            top = std::make_unique<NumberNode>(1.0);
        }

        std::unique_ptr<Node> result = bottom
            ? std::make_unique<BinaryOpNode>(BinaryOperator::DIV, std::move(top), std::move(bottom))
            : std::move(top);

        if (p.numerator == -1.0) { // -1 * x -> -x
            result = std::make_unique<UnaryOpNode>(UnaryOperator::SUB, std::move(result));
        }

        return result;
    }

    /// SUM //////////////////////////////////////////////////////////////////////////////

    // x + 1 - 2 * x  ->  constant 1, terms [(1, x), (-2, x)]
    void flattenSum(const Node& node, double sign, FlatSum& out) {

        if (auto v = node.numericValue()) {
            out.constant += sign * *v;
            return;
        }

        if (auto* bin = dynamic_cast<const BinaryOpNode*>(&node)) {
            if (bin->getOp() == BinaryOperator::ADD || bin->getOp() == BinaryOperator::SUB) {
                flattenSum(bin->getLeft(), sign, out);
                flattenSum(bin->getRight(), bin->getOp() == BinaryOperator::SUB ? -sign : sign, out);
                return;
            }
        }

        if (auto* un = dynamic_cast<const UnaryOpNode*>(&node)) {
            flattenSum(un->getArg(), un->getOp() == UnaryOperator::SUB ? -sign : sign, out);
            return;
        }

        // any other term: split off its numeric coefficient, so that 2 * x and x can be merged
        FlatProduct p;
        flattenProduct(node, false, p);

        if (p.denominator == 0.0 || !std::isfinite(p.numerator) || !std::isfinite(p.denominator)) {
            out.terms.push_back({sign, 1.0, node.clone()}); // e.g. x / 0, leave it alone
            return;
        }

        collectFactors(p);

        if (p.factors.empty()) { // e.g. x / x
            out.constant += sign * p.numerator / p.denominator;
            return;
        }

        double numerator = sign * p.numerator;
        double denominator = p.denominator;
        p.numerator = 1.0;
        p.denominator = 1.0;

        out.terms.push_back({numerator, denominator, buildProduct(p)});
    }

    // x + 1 - x -> 1
    void collectTerms(FlatSum& s) {

        std::vector<Term> merged;

        for (Term& term : s.terms) {
            auto same = std::find_if(merged.begin(), merged.end(),
                [&](const Term& m) { return *m.key == *term.key; });

            if (same != merged.end()) { // a/b + c/d = (ad + cb) / bd
                same->numerator = same->numerator * term.denominator + term.numerator * same->denominator;
                same->denominator *= term.denominator;
                reduceFraction(same->numerator, same->denominator);
            }
            else {
                reduceFraction(term.numerator, term.denominator);
                merged.push_back(std::move(term));
            }
        }

        merged.erase(std::remove_if(merged.begin(), merged.end(),
            [](const Term& t) { return t.numerator == 0.0; }), merged.end());

        std::sort(merged.begin(), merged.end(),
            [](const Term& a, const Term& b) { return canonicalLess(*a.key, *b.key); });

        s.terms = std::move(merged);
    }

    // coefficient * key, e.g. 2/3 and x -> 2 * x / 3
    std::unique_ptr<Node> buildTerm(double numerator, double denominator, const Node& key) {
        FlatProduct p;
        p.numerator = numerator;
        p.denominator = denominator;
        flattenProduct(key, false, p);
        collectFactors(p);
        return buildProduct(p);
    }

    // t1 + t2 - t3 ... + constant
    std::unique_ptr<Node> buildSum(const FlatSum& s) {

        std::unique_ptr<Node> result;

        for (const Term& term : s.terms) {

            if (!result) { // the first term keeps its sign: -2 * x + ...
                result = buildTerm(term.numerator, term.denominator, *term.key);
                continue;
            }

            BinaryOperator op = term.numerator < 0 ? BinaryOperator::SUB : BinaryOperator::ADD;
            result = std::make_unique<BinaryOpNode>(
                op, std::move(result), buildTerm(std::abs(term.numerator), term.denominator, *term.key));
        }

        if (!result) {
            return std::make_unique<NumberNode>(s.constant);
        }

        if (s.constant != 0.0) {
            BinaryOperator op = s.constant < 0 ? BinaryOperator::SUB : BinaryOperator::ADD;
            result = std::make_unique<BinaryOpNode>(
                op, std::move(result), std::make_unique<NumberNode>(std::abs(s.constant)));
        }

        return result;
    }

}

/// PUBLIC FUNCTIONS /////////////////////////////////////////////////////////////////////

std::unique_ptr<Node> simplifySum(const Node& sum) {
    FlatSum s;
    flattenSum(sum, 1.0, s);
    collectTerms(s);
    return buildSum(s);
}

std::unique_ptr<Node> simplifyProduct(const Node& product) {

    FlatProduct p;
    flattenProduct(product, false, p);

    if (p.denominator == 0.0 || !std::isfinite(p.numerator) || !std::isfinite(p.denominator)) {
        return product.clone(); // x / 0 stays as it is
    }

    if (p.numerator == 0.0) { // 0 * f -> 0
        return std::make_unique<NumberNode>(0.0);
    }

    collectFactors(p);
    reduceFraction(p.numerator, p.denominator);

    if (p.factors.empty()) {
        return std::make_unique<NumberNode>(p.numerator / p.denominator);
    }

    return buildProduct(p);
}

std::unique_ptr<Node> Node::simplify() const {

    std::unique_ptr<Node> current = simplifyStep();

    for (int i = 0; i < MAX_PASSES; i++) {
        std::unique_ptr<Node> next = current->simplifyStep();
        if (*next == *current) {
            return next;
        }
        current = std::move(next);
    }

    return current;
}
