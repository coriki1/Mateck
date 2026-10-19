#pragma once

#include "node.h"

// Flatten a +/- or */÷ chain into a list, merge equal terms/factors and rebuild it
// in canonical order. Used by BinaryOpNode::simplifyStep() and UnaryOpNode::simplifyStep().
std::unique_ptr<Node> simplifySum(const Node& sum);
std::unique_ptr<Node> simplifyProduct(const Node& product);

// Decimals that are simple fractions are kept as exact fractions, so 0.5 and 1/2 simplify to the same 1/2.
// Others stay decimal: 9.81 does not become 981/100, 4443055.26 does not become 222152763/50.
constexpr double MAX_FRACTION_DENOMINATOR = 99;
constexpr double MAX_FRACTION_NUMERATOR = 10000;

// the value of a plain number (0.5) or of a number fraction (1/2, -3/4), otherwise empty
std::optional<double> numericFraction(const Node& node);

// v as an exact fraction if its denominator is small (0.5 -> 1/2), otherwise as a number
std::unique_ptr<Node> buildNumber(double v);
