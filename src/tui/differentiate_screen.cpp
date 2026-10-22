#include "screens.h"
#include "format.h"
#include "plot.h"
#include "theme.h"

#include <algorithm>
#include <cmath>
#include <memory>

using namespace ftxui;

namespace {

    const std::vector<std::string> ORDERS = {"1", "2", "3"};  // up to MAX_DERIVATIVE_ORDER

    // d/dx, d²/dx², d³/dx³
    std::string derivativeLabel(const std::string& var, int order) {
        static const std::vector<std::string> powers = {"", "", "²", "³"};
        return "d" + powers[order] + "/d" + var + powers[order];
    }

    // a one-line choice like "x  y  z", the selected entry highlighted
    Component choice(const std::vector<std::string>* entries, int* selected) {
        MenuOption option = MenuOption::Toggle();
        option.entries = entries;
        option.selected = selected;
        option.elements_infix = [] { return text(" "); }; // a │ would join the panel border
        option.entries_option.transform = [](const EntryState& entry) {
            Element e = text(" " + entry.label + " ");
            if (entry.active) {
                e |= color(theme::accent) | bold;
            }
            return entry.focused ? e | inverted : e;
        };
        return Menu(option);
    }

    // f(x0) + f'(x0)(var - x0)
    std::shared_ptr<const ::Node> tangent(const std::string& var, double x0, double y0, double slope) {
        auto offset = std::make_unique<BinaryOpNode>(BinaryOperator::SUB,
            std::make_unique<VarNode>(var), std::make_unique<NumberNode>(x0));
        auto rise = std::make_unique<BinaryOpNode>(BinaryOperator::MUL,
            std::make_unique<NumberNode>(slope), std::move(offset));
        return std::make_shared<BinaryOpNode>(BinaryOperator::ADD,
            std::make_unique<NumberNode>(y0), std::move(rise));
    }

    // built in place (make_shared), never copied: the choices point to its own members
    struct DifferentiateScreen {
        TuiState& state;
        Component input = expressionInput(state);
        std::shared_ptr<VariablesPanel> variables = std::make_shared<VariablesPanel>(state);
        Component history = historyMenu(state, [in = input] { in->TakeFocus(); });

        std::vector<std::string> variableNames = {"x"};
        int variableIndex = 0;
        int orderIndex = 0;
        Component variableChoice = choice(&variableNames, &variableIndex);
        Component orderChoice = choice(&ORDERS, &orderIndex);

        std::string key = "\x01"; // matches no state, so the first frame computes
        Differentiation result;

        explicit DifferentiateScreen(TuiState& state) : state(state) {}
        DifferentiateScreen(const DifferentiateScreen&) = delete;

        void update() {

            std::string chosen = variableNames[std::min<size_t>(variableIndex, variableNames.size() - 1)];

            std::string k = state.expression + '\0' + chosen + '\0' + std::to_string(orderIndex);
            for (const auto& [var, text] : state.values) {
                k += '\0' + var + '=' + text;
            }

            if (k != key) {
                key = k;
                result = differentiateExpression(state.expression, chosen, orderIndex + 1, state.values);

                // the choice follows the variables of the expression
                std::vector<std::string> names = result.assignments.variables;
                if (names.empty()) {
                    names = {"x"};
                }
                if (names != variableNames) {
                    variableNames = names;
                    variableIndex = static_cast<int>(
                        std::find(names.begin(), names.end(), result.variable) - names.begin());
                }
            }

            variables->sync(result.function ? result.assignments.variables : std::vector<std::string>{});
        }

        Element results() const {

            if (!result.function) {
                return hint(result.error ? "fix the expression to see the results" : "nothing to show yet");
            }

            Elements lines = {resultLine("f", format(*result.function))};

            for (size_t i = 0; i < result.derivatives.size(); ++i) {
                lines.push_back(resultLine(derivativeLabel(result.variable, int(i) + 1),
                                           format(*result.derivatives[i]), theme::derivative));
            }
            if (!result.diffError.empty()) {
                lines.push_back(resultLine("", result.diffError, theme::error));
            }

            if (!result.gradient.empty()) {
                lines.push_back(text(""));
                for (const auto& [var, partial] : result.gradient) {
                    lines.push_back(resultLine("∂/∂" + var, format(*partial), theme::derivative));
                }
            }

            if (result.value) {
                lines.push_back(text(""));
                lines.push_back(resultLine("value", formatValue(*result.value), theme::accent));
                for (size_t i = 0; i < result.derivativeValues.size(); ++i) {
                    lines.push_back(resultLine(derivativeLabel(result.variable, int(i) + 1) + " value",
                                               formatValue(result.derivativeValues[i]), theme::accent));
                }
            }

            return vbox(std::move(lines));
        }

        Element plot() const {

            if (!result.function) {
                return hint("nothing to plot");
            }

            const std::string& var = result.variable;
            std::shared_ptr<const ::Node> f = result.assignments.apply(*result.function, var)->simplify();

            for (const std::string& other : ::variables(*f)) { // the member hides the function
                if (other != var) {
                    return hint("assign " + other + " to plot by " + var);
                }
            }

            std::vector<Curve> curves = {{f, theme::plotFunction}};
            if (!result.derivatives.empty()) {
                curves.push_back({result.assignments.apply(*result.derivatives.back(), var)->simplify(),
                                  theme::plotDerivative});
            }

            // the tangent at the given point, if every variable has a value
            std::optional<PlotPoint> point;
            for (const auto& [name, value] : result.assignments.values) {
                if (name == var && result.value && !result.derivativeValues.empty()) {
                    double x0 = value->evaluate({});
                    double y0 = *result.value;
                    double slope = result.derivativeValues[0];
                    if (std::isfinite(y0) && std::isfinite(slope)) {
                        curves.push_back({tangent(var, x0, y0, slope), theme::plotTangent, false});
                    }
                    point = PlotPoint{x0, y0, theme::accent};
                }
            }

            double w = state.plotHalfWidth;
            Element legend = hbox({
                text("── f  ") | color(theme::plotFunction),
                text("── " + derivativeLabel(var, int(result.derivatives.size())) + "  ")
                    | color(theme::plotDerivative),
                text(point ? "── tangent" : "") | color(theme::plotTangent),
            });
            return vbox({plotView(curves, var, {-w, w}, point) | flex, legend});
        }

        Element render() {
            update();

            Element choices = hbox({
                text("by ") | color(theme::label), variableChoice->Render(),
                text("   order ") | color(theme::label), orderChoice->Render(),
            });

            Element left = vbox({
                panel("Derivative", vbox({choices, text(""), results()}),
                      variableChoice->Focused() || orderChoice->Focused()),
                panel("Variables", variables->view(result.assignments), variables->Focused()),
                panel("History", history->Render() | frame | flex, history->Focused()) | flex,
            });

            Element right;
            if (state.showPlot) {
                right = plot();
            }
            else {
                right = result.derivatives.empty() ? hint("nothing to show yet")
                                                   : treeView(*result.derivatives.back());
            }
            Element body = splitBody(left, rightPanel(state, right));

            return vbox({
                expressionBox(input, result.error),
                body | flex,
                statusBar("Tab focus   ←→ choose   ↑↓ history   F2 tree/plot   PgUp/PgDn zoom   Esc menu"),
            });
        }
    };

}

Component differentiateScreen(TuiState& state) {

    auto screen = std::make_shared<DifferentiateScreen>(state);

    std::vector<Component> zones = {screen->input, screen->variableChoice, screen->orderChoice,
                                    screen->variables, screen->history};
    Component layout = Renderer(Container::Vertical(zones), [screen] { return screen->render(); });

    return screenKeys(layout, state, zones);
}
