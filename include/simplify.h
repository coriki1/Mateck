#pragma once

#include "node.h"

// Flatten a +/- or */÷ chain into a list, merge equal terms/factors and rebuild it
// in canonical order. Used by BinaryOpNode::simplifyStep() and UnaryOpNode::simplifyStep().
std::unique_ptr<Node> simplifySum(const Node& sum);
std::unique_ptr<Node> simplifyProduct(const Node& product);
