enum class BINARYOP {
    ADD,
    SUB,
    MUL,
    DIV,
    POW,
};

enum class UNARYOP {
    UADD,
    USUB
};

enum class FUNC {

    ////// TRIGONOMETRIC FUNCTIONS //////
    SIN, COS, TAN, COT,
    ARCSIN, ARCCOS, ARCTAN, ARCCOT,

    ////// HYPERBOLIC FUNCTIONS //////
    SINH, COSH, TANH, COTH,
    ASINH, ACOSH, ATANH, ACOTH,

    ////// MISC. //////
    LN, LOG, SQRT, ABS,
    POW, FLOOR, CEIL, ROUND

};