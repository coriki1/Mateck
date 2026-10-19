#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <vector>

#include "simplify.h"
#include "compare.h"

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

    // t1 + t2 + ... + constantNumerator / constantDenominator
    struct FlatSum {
        double constantNumerator = 0.0;
        double constantDenominator = 1.0;
        std::vector<Term> terms;
    };

    /// HELPERS //////////////////////////////////////////////////////////////////////////

    bool isInteger(double v) {
        return std::abs(v) < 1e15 && v == std::round(v);
    }

    // 0.75 -> 3/4: the smallest denominator up to MAX_FRACTION_DENOMINATOR that gives v
    // with a numerator up to MAX_FRACTION_NUMERATOR,
    // allowing for the rounding error of decimals like 0.1, which has no exact binary form
    bool smallFraction(double v, double& numerator, double& denominator) {
        for (double q = 1; q <= MAX_FRACTION_DENOMINATOR; q++) {
            double p = std::round(v * q);
            if (std::abs(v * q - p) <= 1e-12 * std::max(1.0, std::abs(v * q)) && std::abs(p) <= MAX_FRACTION_NUMERATOR) {
                numerator = p;
                denominator = q;
                return true;
            }
        }
        return false;
    }

    // 6/4 -> 3/2, 0.5/1 -> 1/2; keeps the fraction exact while both parts are integers
    void reduceFraction(double& numerator, double& denominator) {

        if (isInteger(numerator) && isInteger(denominator)) {
            long long g = std::gcd(std::llround(numerator), std::llround(denominator));
            if (g > 1) {
                numerator /= g;
                denominator /= g;
            }
        }
        else if (!smallFraction(numerator / denominator, numerator, denominator)) {
            numerator /= denominator; // 9.81 or sin(1) stays a decimal
            denominator = 1.0;
        }

        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
    }

    // a/b + c/d = (ad + cb) / bd
    void addFraction(double& numerator, double& denominator, double addNumerator, double addDenominator) {
        numerator = numerator * addDenominator + addNumerator * denominator;
        denominator *= addDenominator;
        reduceFraction(numerator, denominator);
    }

    // exact number: 3 or 1 / 2
    std::unique_ptr<Node> buildFraction(double numerator, double denominator) {
        if (denominator == 1.0) {
            return std::make_unique<NumberNode>(numerator);
        }
        return std::make_unique<BinaryOpNode>(BinaryOperator::DIV,
            std::make_unique<NumberNode>(numerator), std::make_unique<NumberNode>(denominator));
    }

    std::unique_ptr<Node> multiply(std::unique_ptr<Node> acc, std::unique_ptr<Node> factor) {
        if (!acc) {
            return factor;
        }
        return std::make_unique<BinaryOpNode>(BinaryOperator::MUL, std::move(acc), std::move(factor));
    }

    void flattenSum(const Node& node, double sign, FlatSum& out);
    void collectTerms(FlatSum& s);
    std::unique_ptr<Node> buildSum(const FlatSum& s);

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

                case BinaryOperator::POW: // x^2 -> factor x with exponent 2, x^(1/2) -> exponent 0.5
                    if (auto k = numericFraction(bin->getRight())) {
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

    bool isSum(const Node& n) {
        auto* bin = dynamic_cast<const BinaryOpNode*>(&n);
        return bin && (bin->getOp() == BinaryOperator::ADD || bin->getOp() == BinaryOperator::SUB);
    }

    // -x + y -> -(x - y): a sum factor never starts with a negative term, so that
    // -(A * B) and (-A) * B end up the same; returns false if the sum was left as it is
    bool negateIfLeadingTermNegative(std::unique_ptr<Node>& sum) {

        FlatSum s;
        flattenSum(*sum, 1.0, s);
        collectTerms(s);

        bool negative = s.terms.empty() ? s.constantNumerator < 0 : s.terms[0].numerator < 0;
        if (!negative) {
            return false;
        }

        FlatSum negated;
        flattenSum(*sum, -1.0, negated);
        collectTerms(negated);
        sum = buildSum(negated);
        return true;
    }

    // x * x^2 / x -> x^2
    void collectFactors(FlatProduct& p) {

        // pull the sign out of sum factors; (-S)^k = (-1)^k S^k only holds for integer k
        for (auto& [factor, exponent] : p.factors) {
            bool integerExponent = exponent == std::round(exponent);
            if (isSum(*factor) && integerExponent && negateIfLeadingTermNegative(factor)) {
                if (std::fmod(std::abs(exponent), 2.0) == 1.0) {
                    p.numerator = -p.numerator;
                }
            }
        }

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
            [](const auto& a, const auto& b) { return compare(*a.first, *b.first) < 0; });

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
                : std::make_unique<BinaryOpNode>(BinaryOperator::POW, factor->clone(), buildNumber(k));

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
            addFraction(out.constantNumerator, out.constantDenominator, sign * *v, 1.0);
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

        if (p.factors.empty()) { // e.g. 1 / 2 or x / x
            addFraction(out.constantNumerator, out.constantDenominator, sign * p.numerator, p.denominator);
            return;
        }

        double numerator = sign * p.numerator;
        double denominator = p.denominator;
        p.numerator = 1.0;
        p.denominator = 1.0;

        out.terms.push_back({numerator, denominator, buildProduct(p)});
    }

    // exponents of the plain variables in a term, e.g. x^2 * y * sin(x) -> {x: 2, y: 1}
    std::map<std::string, double> monomial(const Node& key) {
        FlatProduct p;
        flattenProduct(key, false, p);

        std::map<std::string, double> exponents;
        for (const auto& [factor, exponent] : p.factors) {
            if (auto* var = dynamic_cast<const VarNode*>(factor.get())) {
                exponents[var->getName()] += exponent;
            }
        }
        return exponents;
    }

    double totalDegree(const std::map<std::string, double>& exponents) {
        double degree = 0.0;
        for (const auto& entry : exponents) {
            degree += entry.second;
        }
        return degree;
    }

    // polynomial order of terms: higher total degree first (x^3 before x), then the exponent
    // of each variable in alphabetical order (x^2 before x*y before y^2), finally compare()
    bool termLess(const Term& a, const Term& b) {

        std::map<std::string, double> ma = monomial(*a.key);
        std::map<std::string, double> mb = monomial(*b.key);

        double da = totalDegree(ma);
        double db = totalDegree(mb);
        if (da != db) {
            return da > db;
        }

        // walk both maps together, in alphabetical order of the variable names
        auto ia = ma.begin();
        auto ib = mb.begin();
        while (ia != ma.end() || ib != mb.end()) {

            bool takeA = ib == mb.end() || (ia != ma.end() && ia->first <= ib->first);
            const std::string& name = takeA ? ia->first : ib->first;

            double ea = (ia != ma.end() && ia->first == name) ? (ia++)->second : 0.0;
            double eb = (ib != mb.end() && ib->first == name) ? (ib++)->second : 0.0;
            if (ea != eb) {
                return ea > eb;
            }
        }

        return compare(*a.key, *b.key) < 0;
    }

    // x + 1 - x -> 1
    void collectTerms(FlatSum& s) {

        std::vector<Term> merged;

        for (Term& term : s.terms) {
            auto same = std::find_if(merged.begin(), merged.end(),
                [&](const Term& m) { return *m.key == *term.key; });

            if (same != merged.end()) {
                addFraction(same->numerator, same->denominator, term.numerator, term.denominator);
            }
            else {
                reduceFraction(term.numerator, term.denominator);
                merged.push_back(std::move(term));
            }
        }

        merged.erase(std::remove_if(merged.begin(), merged.end(),
            [](const Term& t) { return t.numerator == 0.0; }), merged.end());

        std::sort(merged.begin(), merged.end(),
            [](const Term& a, const Term& b) { return termLess(a, b); });

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
            return buildFraction(s.constantNumerator, s.constantDenominator);
        }

        if (s.constantNumerator != 0.0) {
            BinaryOperator op = s.constantNumerator < 0 ? BinaryOperator::SUB : BinaryOperator::ADD;
            result = std::make_unique<BinaryOpNode>(
                op, std::move(result), buildFraction(std::abs(s.constantNumerator), s.constantDenominator));
        }

        return result;
    }

}

/// PUBLIC FUNCTIONS /////////////////////////////////////////////////////////////////////

std::optional<double> numericFraction(const Node& node) {

    if (auto v = node.numericValue()) {
        return v;
    }

    auto* bin = dynamic_cast<const BinaryOpNode*>(&node);
    if (bin && bin->getOp() == BinaryOperator::DIV) {
        auto numerator = bin->getLeft().numericValue();
        auto denominator = bin->getRight().numericValue();
        if (numerator && denominator && *denominator != 0.0) {
            return *numerator / *denominator;
        }
    }

    return std::nullopt;
}

std::unique_ptr<Node> buildNumber(double v) {

    double numerator = v;
    double denominator = 1.0;
    if (!isInteger(v) && smallFraction(v, numerator, denominator)) {
        return buildFraction(numerator, denominator);
    }
    return std::make_unique<NumberNode>(v);
}

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

    if (p.factors.empty()) { // 6 / 4 -> 3 / 2
        return buildFraction(p.numerator, p.denominator);
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
