#include "compare.h"

namespace {

    // position of the node kind in the order
    int rank(const Node& n) {
        if (n.numericValue())                       return 0;
        if (dynamic_cast<const ConstantNode*>(&n))  return 1;
        if (dynamic_cast<const VarNode*>(&n))       return 2;
        if (dynamic_cast<const FuncNode*>(&n))      return 3;
        if (dynamic_cast<const UnaryOpNode*>(&n))   return 4;
        return 5;                                   // BinaryOpNode
    }

    int sign(int v) {
        return (v > 0) - (v < 0);
    }

    template <typename T>
    int compareValues(const T& a, const T& b) {
        if (a < b) return -1;
        if (b < a) return 1;
        return 0;
    }

}

int compare(const Node& a, const Node& b) {

    int byRank = compareValues(rank(a), rank(b));
    if (byRank != 0) {
        return byRank;
    }

    switch (rank(a)) {

        case 0: // numbers
            return compareValues(*a.numericValue(), *b.numericValue());

        case 1: // constants
            return sign(static_cast<const ConstantNode&>(a).getName()
                        .compare(static_cast<const ConstantNode&>(b).getName()));

        case 2: // variables
            return sign(static_cast<const VarNode&>(a).getName()
                        .compare(static_cast<const VarNode&>(b).getName()));

        case 3: { // functions: by name, so cos comes before sin
            auto& fa = static_cast<const FuncNode&>(a);
            auto& fb = static_cast<const FuncNode&>(b);
            int byName = sign(functionTypeToString(fa.getFunc()).compare(functionTypeToString(fb.getFunc())));
            return byName != 0 ? byName : compare(fa.getArg(), fb.getArg());
        }

        case 4: { // unary operators
            auto& ua = static_cast<const UnaryOpNode&>(a);
            auto& ub = static_cast<const UnaryOpNode&>(b);
            int byOp = compareValues(static_cast<int>(ua.getOp()), static_cast<int>(ub.getOp()));
            return byOp != 0 ? byOp : compare(ua.getArg(), ub.getArg());
        }

        default: { // binary operators
            auto& ba = static_cast<const BinaryOpNode&>(a);
            auto& bb = static_cast<const BinaryOpNode&>(b);
            int byOp = compareValues(static_cast<int>(ba.getOp()), static_cast<int>(bb.getOp()));
            if (byOp != 0) {
                return byOp;
            }
            int byLeft = compare(ba.getLeft(), bb.getLeft());
            return byLeft != 0 ? byLeft : compare(ba.getRight(), bb.getRight());
        }
    }
}
