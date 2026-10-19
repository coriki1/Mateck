#pragma once

#include "node.h"

#include <string>

// Readable form of an expression for the user, e.g. x^3 - 2x, 2x cos(x^2), 5x/6.
// Unlike toString() it uses implicit multiplication and compact fractions, but the
// result can still be parsed back: parse(format(f)) has the same value as f.
//
//   2 * x         -> 2x          number directly before a name or '('
//   x * y         -> x y         a space everywhere else ("xy" could merge into a name)
//   x * 3, x * -y -> x * 3       '*' stays where the right side starts with a number or sign
//   5 * x / 6     -> 5x/6        division without spaces
std::string format(const Node& node);

// a number with up to 10 significant digits and never in scientific notation,
// so the parser can read it back: 6.283185307, 0.000000000001
std::string formatNumber(double v);
