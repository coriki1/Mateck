#pragma once

#include "node.h"

// Expanded and simplified form of an expression: products are multiplied out over sums
// and sums raised to a positive integer power are written out:
//
//   (x + 1)(x - 1)  -> x^2 - 1
//   (x + 1)^2       -> x^2 + 2x + 1
//   sin((x + 1)^2)  -> sin(x^2 + 2x + 1)      function arguments are expanded too
//   x/(x + 1)       -> x/(x + 1)              denominators are not split up
//   (x + 1)^-1, (x + 1)^(1/2)                 stay as they are
//
// Returns nullptr if the result would have more than MAX_EXPANDED_TERMS terms,
// e.g. for (x + 1)^100, so huge expansions stop early instead of running for minutes.
constexpr size_t MAX_EXPANDED_TERMS = 50;

std::unique_ptr<Node> expand(const Node& node);
