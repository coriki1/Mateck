#include <gtest/gtest.h>

#include "helpers.h"
#include "parser.h"
#include "tree.h"

namespace {

    std::string tree(const std::string& input) {
        std::string out;
        for (const std::string& line : drawTree(*parse(input))) {
            out += line + "\n";
        }
        return out;
    }

}

TEST(Tree, SingleNode) {
    EXPECT_EQ(tree("x"), "x\n");
    EXPECT_EQ(tree("2.5"), "2.5\n");
    EXPECT_EQ(tree("pi"), "pi\n");
}

TEST(Tree, NestedBranches) {
    EXPECT_EQ(tree("2x sin x^2"),
        "*\n"
        "├── *\n"
        "│   ├── 2\n"
        "│   └── x\n"
        "└── sin\n"
        "    └── ^\n"
        "        ├── x\n"
        "        └── 2\n");
}

TEST(Tree, UnaryMinus) {
    EXPECT_EQ(tree("-x - 1"),
        "-\n"
        "├── -\n"
        "│   └── x\n"
        "└── 1\n");
}

TEST(Tree, OneLinePerNode) {
    std::unique_ptr<Node> expr = parse("(x + 1)^2 / ln(y)");
    // +, x, 1, ^, 2, /, ln, y
    EXPECT_EQ(drawTree(*expr).size(), 8u);
}
