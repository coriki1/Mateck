#pragma once

#include "node.h"

#include <string>
#include <vector>

// The expression tree drawn line by line, like the output of the `tree` command:
//
//   2x sin(x^2)  ->  *
//                    ├── *
//                    │   ├── 2
//                    │   └── x
//                    └── sin
//                        └── ^
//                            ├── x
//                            └── 2
//
// One string per line, so a caller can scroll or cut it to fit.
std::vector<std::string> drawTree(const Node& node);
