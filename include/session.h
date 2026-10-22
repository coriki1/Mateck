#pragma once

#include "node.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/// SESSION /////////////////////////////////////////////////////////////////////////////
//
// The calculations behind the screens of the terminal UI, kept free of FTXUI so they can
// be unit-tested. Each function takes the text the user typed and returns everything a
// screen shows; the screens only draw the result.

// what the user typed as the value of each variable; empty text means "no value"
using ValueInputs = std::map<std::string, std::string>;

// the expression could not be parsed; column is 0-based and set if the error points to a place
struct InputError {
    std::string message;
    std::optional<size_t> column;
};

// the typed values that are valid, checked against the variables of an expression
struct Assignments {
    std::vector<std::string> variables;                                // in order of first appearance
    std::vector<std::pair<std::string, std::unique_ptr<Node>>> values; // the valid, non-empty ones
    std::map<std::string, std::string> errors;                         // variable -> why its value is invalid

    // every variable has a value
    bool complete() const { return values.size() == variables.size(); }

    // copy of node with the values substituted, except for the variable named except
    std::unique_ptr<Node> apply(const Node& node, const std::string& except = "") const;
};

/// EVALUATION SCREEN ///

struct Evaluation {
    std::optional<InputError> error;     // set if parsing failed; nothing below is set then
    std::unique_ptr<Node> parsed;
    std::unique_ptr<Node> simplified;
    std::unique_ptr<Node> expanded;      // null if it would be too long, see expand.h
    Assignments assignments;
    std::unique_ptr<Node> substituted;   // simplified; set if some but not all variables have a value
    std::optional<double> value;         // set if every variable has a value; NaN where undefined
};

Evaluation evaluateExpression(const std::string& text, const ValueInputs& inputs);

/// DIFFERENTIATION SCREEN ///

constexpr int MAX_DERIVATIVE_ORDER = 3;

struct Differentiation {
    std::optional<InputError> error;     // set if parsing failed; nothing below is set then
    std::unique_ptr<Node> function;      // simplified
    std::string variable;                // the variable actually used, see differentiateExpression()
    std::vector<std::unique_ptr<Node>> derivatives;  // simplified: f', f'', ... up to the order
    std::string diffError;               // set if differentiating stopped, e.g. at floor(x)
    std::vector<std::pair<std::string, std::unique_ptr<Node>>> gradient; // simplified partial
                                         // derivatives, set if there is more than one variable
    Assignments assignments;
    std::optional<double> value;         // f at the point, set if every variable has a value
    std::vector<double> derivativeValues;  // f', f'', ... at the same point
};

// Differentiates order times (1 to MAX_DERIVATIVE_ORDER) by variable. If the expression
// does not contain variable, its first variable is used instead (x if it has none).
Differentiation differentiateExpression(const std::string& text, const std::string& variable,
                                        int order, const ValueInputs& inputs);

/// PLOTTING ///

struct Range {
    double min, max;
};

// n evenly spaced samples of f by var over [range.min, range.max]; NaN where f is undefined
// or contains a variable other than var
std::vector<double> samplePlot(const Node& f, const std::string& var, Range range, int n);

// the y range to show for samples: NaNs are skipped, and near poles like tan x the few
// huge values are cut off so the rest of the curve stays visible. Empty if nothing is finite.
std::optional<Range> plotRange(const std::vector<double>& samples);

/// NUMBERS ///

// a computed value for display: "undefined" if not finite, 0 for values like 6.1e-17
std::string formatValue(double v);
