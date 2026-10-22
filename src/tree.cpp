#include "tree.h"

namespace {

    // prefix: what the lines of this subtree start with, built from the levels above
    void drawChildren(const Node& node, const std::string& prefix, std::vector<std::string>& lines) {

        std::vector<const Node*> children = node.children();

        for (size_t i = 0; i < children.size(); ++i) {
            bool last = i + 1 == children.size();

            lines.push_back(prefix + (last ? "└── " : "├── ") + children[i]->label());

            // below the last child there is nothing more to connect, so no vertical line
            drawChildren(*children[i], prefix + (last ? "    " : "│   "), lines);
        }
    }

}

std::vector<std::string> drawTree(const Node& node) {
    std::vector<std::string> lines = {node.label()};
    drawChildren(node, "", lines);
    return lines;
}
