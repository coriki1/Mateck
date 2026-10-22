#include "session.h"
#include "errors.h"
#include "expand.h"
#include "format.h"
#include "parser.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

    // values closer to 0 than this are shown as 0, so cos(pi/2) prints 0 instead of 6.12323e-17
    constexpr double ZERO_THRESHOLD = 1e-12;

    // plotRange() cuts off the outliers when the full range is this many times wider
    // than the range of the middle 90% of the samples
    constexpr double OUTLIER_RATIO = 10;

    // empty space above and below the curve, as a fraction of its height
    constexpr double PLOT_MARGIN = 0.05;

    constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

    bool isBlank(const std::string& text) {
        return text.find_first_not_of(" \t") == std::string::npos;
    }

    // parses the expression, or returns why it failed
    std::unique_ptr<Node> parseInput(const std::string& text, std::optional<InputError>& error) {
        try {
            return parse(text);
        }
        catch (const PositionedError& e) {
            error = InputError{e.what(), e.getPosition()};
        }
        catch (const MateckError& e) {
            error = InputError{e.what(), std::nullopt};
        }
        return nullptr;
    }

    Assignments readAssignments(const Node& expr, const ValueInputs& inputs) {

        Assignments result;
        result.variables = variables(expr);

        for (const std::string& var : result.variables) {
            auto it = inputs.find(var);
            if (it == inputs.end() || isBlank(it->second)) {
                continue;
            }

            try {
                std::unique_ptr<Node> value = parse(it->second);

                std::vector<std::string> inner = variables(*value);
                if (!inner.empty()) {
                    result.errors[var] = "the value cannot contain a variable (" + inner[0] + ")";
                    continue;
                }

                result.values.emplace_back(var, std::move(value));
            }
            catch (const MateckError& e) {
                result.errors[var] = e.what();
            }
        }

        return result;
    }

    // the sample at index i of n, evenly spaced over the range including both ends
    double sampleAt(Range range, int i, int n) {
        if (n == 1) {
            return range.min;
        }
        return range.min + (range.max - range.min) * i / (n - 1);
    }

}

std::unique_ptr<Node> Assignments::apply(const Node& node, const std::string& except) const {

    std::unique_ptr<Node> result = node.clone();

    for (const auto& [var, value] : values) {
        if (var != except) {
            result = result->substitute(var, *value);
        }
    }

    return result;
}

/// EVALUATION ///

Evaluation evaluateExpression(const std::string& text, const ValueInputs& inputs) {

    Evaluation result;

    result.parsed = parseInput(text, result.error);
    if (!result.parsed) {
        return result;
    }

    result.simplified = result.parsed->simplify();
    result.expanded = expand(*result.parsed);
    result.assignments = readAssignments(*result.parsed, inputs);

    if (result.assignments.complete()) {
        result.value = result.assignments.apply(*result.parsed)->evaluate({});
    }
    else if (!result.assignments.values.empty()) {
        result.substituted = result.assignments.apply(*result.parsed)->simplify();
    }

    return result;
}

/// DIFFERENTIATION ///

Differentiation differentiateExpression(const std::string& text, const std::string& variable,
                                        int order, const ValueInputs& inputs) {

    Differentiation result;

    std::unique_ptr<Node> parsed = parseInput(text, result.error);
    if (!parsed) {
        return result;
    }

    result.function = parsed->simplify();
    result.assignments = readAssignments(*parsed, inputs);

    const std::vector<std::string>& vars = result.assignments.variables;
    if (std::find(vars.begin(), vars.end(), variable) != vars.end()) {
        result.variable = variable;
    }
    else {
        result.variable = vars.empty() ? "x" : vars[0];
    }

    order = std::clamp(order, 1, MAX_DERIVATIVE_ORDER);

    try {
        std::unique_ptr<Node> current = parsed->clone();
        for (int i = 0; i < order; ++i) {
            current = current->differentiate(result.variable)->simplify();
            result.derivatives.push_back(current->clone());
        }

        if (vars.size() > 1) {
            for (const std::string& var : vars) {
                result.gradient.emplace_back(var, parsed->differentiate(var)->simplify());
            }
        }
    }
    catch (const DiffError& e) {
        result.diffError = e.what();
        result.gradient.clear(); // a gradient with a missing partial derivative would mislead
    }

    if (result.assignments.complete()) {
        result.value = result.assignments.apply(*parsed)->evaluate({});
        for (const auto& derivative : result.derivatives) {
            result.derivativeValues.push_back(result.assignments.apply(*derivative)->evaluate({}));
        }
    }

    return result;
}

/// PLOTTING ///

std::vector<double> samplePlot(const Node& f, const std::string& var, Range range, int n) {

    std::vector<double> samples;
    samples.reserve(n);

    for (int i = 0; i < n; ++i) {
        try {
            samples.push_back(f.evaluate({{var, sampleAt(range, i, n)}}));
        }
        catch (const EvalError&) { // another variable without a value
            samples.push_back(NaN);
        }
    }

    return samples;
}

std::optional<Range> plotRange(const std::vector<double>& samples) {

    std::vector<double> finite;
    for (double v : samples) {
        if (std::isfinite(v)) {
            finite.push_back(v);
        }
    }

    if (finite.empty()) {
        return std::nullopt;
    }

    std::sort(finite.begin(), finite.end());

    Range range{finite.front(), finite.back()};

    // the middle 90%: if the full range is much wider, a few values near a pole stretch it
    double low  = finite[finite.size() * 5 / 100];
    double high = finite[finite.size() * 95 / 100];
    if (high > low && range.max - range.min > OUTLIER_RATIO * (high - low)) {
        range = {low, high};
    }

    if (range.max == range.min) { // a constant: put the line in the middle
        range.min -= 1;
        range.max += 1;
    }

    double margin = (range.max - range.min) * PLOT_MARGIN;
    return Range{range.min - margin, range.max + margin};
}

/// NUMBERS ///

std::string formatValue(double v) {
    if (!std::isfinite(v)) { // sqrt(-1), 1/0
        return "undefined";
    }
    if (std::abs(v) < ZERO_THRESHOLD) {
        return "0";
    }
    return formatNumber(v);
}
