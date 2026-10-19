#include "expand.h"
#include "simplify.h"

#include <cmath>
#include <vector>

namespace {

    // the terms of a sum: x^2 - 2x + 1 -> [x^2, -(2x), 1]
    using Terms = std::vector<std::unique_ptr<Node>>;

    // (x + 1)^n with a larger n can never fit into MAX_EXPANDED_TERMS terms
    constexpr double MAX_EXPANDED_POWER = 1000;

    std::unique_ptr<Node> negate(std::unique_ptr<Node> n) {
        return std::make_unique<UnaryOpNode>(UnaryOperator::SUB, std::move(n));
    }

    // splits a sum into its terms, the sign goes into the term: x - 2y -> [x, -(2y)]
    void splitSum(const Node& n, bool negative, Terms& out) {

        if (auto* bin = dynamic_cast<const BinaryOpNode*>(&n)) {
            if (bin->getOp() == BinaryOperator::ADD || bin->getOp() == BinaryOperator::SUB) {
                splitSum(bin->getLeft(), negative, out);
                splitSum(bin->getRight(), negative != (bin->getOp() == BinaryOperator::SUB), out);
                return;
            }
        }

        if (auto* un = dynamic_cast<const UnaryOpNode*>(&n)) {
            splitSum(un->getArg(), negative != (un->getOp() == UnaryOperator::SUB), out);
            return;
        }

        out.push_back(negative ? negate(n.clone()) : n.clone());
    }

    std::unique_ptr<Node> sumOf(const Terms& terms) {

        if (terms.empty()) {
            return std::make_unique<NumberNode>(0.0);
        }

        std::unique_ptr<Node> sum = terms[0]->clone();
        for (size_t i = 1; i < terms.size(); i++) {
            sum = std::make_unique<BinaryOpNode>(BinaryOperator::ADD, std::move(sum), terms[i]->clone());
        }
        return sum;
    }

    // merges like terms (x + x -> 2x) by simplifying the sum and splitting it again;
    // false if more than MAX_EXPANDED_TERMS terms remain
    bool mergeTerms(Terms& terms) {

        Terms merged;
        splitSum(*sumOf(terms)->simplify(), false, merged);

        if (merged.size() > MAX_EXPANDED_TERMS) {
            return false;
        }
        terms = std::move(merged);
        return true;
    }

    // (a + b)(c + d) -> ac + ad + bc + bd
    bool multiply(const Terms& left, const Terms& right, Terms& out) {

        Terms products;
        for (const auto& l : left) {
            for (const auto& r : right) {
                products.push_back(std::make_unique<BinaryOpNode>(BinaryOperator::MUL, l->clone(), r->clone()));
            }
        }

        if (!mergeTerms(products)) {
            return false;
        }
        out = std::move(products);
        return true;
    }

    bool expandTerms(const Node& node, Terms& out);

    // a part that stays together, like a function argument or a denominator
    std::unique_ptr<Node> expandWhole(const Node& node) {
        Terms terms;
        if (!expandTerms(node, terms)) {
            return nullptr;
        }
        return sumOf(terms);
    }

    bool expandBinary(const BinaryOpNode& bin, Terms& out) {

        Terms left;
        if (!expandTerms(bin.getLeft(), left)) {
            return false;
        }

        switch (bin.getOp()) {

            case BinaryOperator::ADD:
            case BinaryOperator::SUB: {
                Terms right;
                if (!expandTerms(bin.getRight(), right)) {
                    return false;
                }
                for (auto& term : right) {
                    left.push_back(bin.getOp() == BinaryOperator::SUB ? negate(std::move(term)) : std::move(term));
                }
                if (!mergeTerms(left)) {
                    return false;
                }
                out = std::move(left);
                return true;
            }

            case BinaryOperator::MUL: {
                Terms right;
                return expandTerms(bin.getRight(), right) && multiply(left, right, out);
            }

            case BinaryOperator::DIV: { // (a + b)/c -> a/c + b/c, the denominator stays whole
                std::unique_ptr<Node> denominator = expandWhole(bin.getRight());
                if (!denominator) {
                    return false;
                }
                Terms quotients;
                for (const auto& term : left) {
                    quotients.push_back(std::make_unique<BinaryOpNode>(BinaryOperator::DIV, term->clone(), denominator->clone()));
                }
                if (!mergeTerms(quotients)) {
                    return false;
                }
                out = std::move(quotients);
                return true;
            }

            case BinaryOperator::POW: {
                std::optional<double> k = numericFraction(*bin.getRight().simplify()); // (x + 1)^(1 + 1) too
                bool expandable = k && *k >= 1 && *k == std::round(*k) && left.size() > 1;

                if (!expandable) { // x^5, (x + 1)^-1, (x + 1)^(1/2), x^y: one term
                    std::unique_ptr<Node> exponent = expandWhole(bin.getRight());
                    if (!exponent) {
                        return false;
                    }
                    out.clear();
                    out.push_back(std::make_unique<BinaryOpNode>(BinaryOperator::POW, sumOf(left), std::move(exponent)));
                    return true;
                }

                if (*k > MAX_EXPANDED_POWER) {
                    return false;
                }

                // (x + 1)^3 = (x + 1)(x + 1)(x + 1), merging after every step keeps the lists short
                Terms power;
                for (const auto& term : left) {
                    power.push_back(term->clone());
                }
                for (int i = 1; i < static_cast<int>(*k); i++) {
                    if (!multiply(power, left, power)) {
                        return false;
                    }
                }
                out = std::move(power);
                return true;
            }
        }

        return false;
    }

    bool expandTerms(const Node& node, Terms& out) {

        out.clear();

        if (auto* bin = dynamic_cast<const BinaryOpNode*>(&node)) {
            return expandBinary(*bin, out);
        }

        if (auto* un = dynamic_cast<const UnaryOpNode*>(&node)) { // -(a + b) -> -a - b
            Terms inner;
            if (!expandTerms(un->getArg(), inner)) {
                return false;
            }
            for (auto& term : inner) {
                out.push_back(un->getOp() == UnaryOperator::SUB ? negate(std::move(term)) : std::move(term));
            }
            return true;
        }

        if (auto* func = dynamic_cast<const FuncNode*>(&node)) { // sin((x + 1)^2) -> sin(x^2 + 2x + 1)
            std::unique_ptr<Node> argument = expandWhole(func->getArg());
            if (!argument) {
                return false;
            }
            out.push_back(std::make_unique<FuncNode>(func->getFunc(), std::move(argument)));
            return true;
        }

        out.push_back(node.clone()); // numbers, constants, variables
        return true;
    }

}

std::unique_ptr<Node> expand(const Node& node) {

    Terms terms;
    if (!expandTerms(node, terms)) {
        return nullptr;
    }

    std::unique_ptr<Node> result = sumOf(terms)->simplify();

    Terms check;
    splitSum(*result, false, check);
    if (check.size() > MAX_EXPANDED_TERMS) {
        return nullptr;
    }
    return result;
}
