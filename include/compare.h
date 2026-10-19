#pragma once

#include "node.h"

// Total order of expression trees: negative if a comes before b, 0 if they are
// structurally equal (a == b), positive otherwise.
//
// Node kinds come in this order: numbers, constants, variables, functions,
// unary operators, binary operators. Within a kind: numbers by value, constants,
// variables and functions by name, then the children from left to right.
// simplify() uses it to order factors, so products read like 2 pi x sin(x) (x + 1).
int compare(const Node& a, const Node& b);
