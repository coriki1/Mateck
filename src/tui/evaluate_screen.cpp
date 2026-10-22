#include "screens.h"
#include "format.h"
#include "plot.h"
#include "theme.h"

#include <algorithm>
#include <memory>

using namespace ftxui;

namespace {

    // the inputs the evaluation was computed from, so it is only recomputed when they change
    std::string cacheKey(const TuiState& state) {
        std::string key = state.expression;
        for (const auto& [var, text] : state.values) {
            key += '\0' + var + '=' + text;
        }
        return key;
    }

    struct EvaluateScreen {
        TuiState& state;
        Component input = expressionInput(state);
        std::shared_ptr<VariablesPanel> variables = std::make_shared<VariablesPanel>(state);
        Component history = historyMenu(state, [in = input] { in->TakeFocus(); });

        std::string key = "\x01"; // matches no state, so the first frame computes
        Evaluation result;

        explicit EvaluateScreen(TuiState& state) : state(state) {}

        void update() {
            std::string k = cacheKey(state);
            if (k != key) {
                key = k;
                result = evaluateExpression(state.expression, state.values);
            }
            variables->sync(result.parsed ? result.assignments.variables : std::vector<std::string>{});
        }

        Element results() const {

            if (!result.parsed) {
                return hint(result.error ? "fix the expression to see the results" : "nothing to show yet");
            }

            Elements lines = {
                resultLine("parsed", format(*result.parsed)),
                resultLine("simplified", format(*result.simplified)),
                resultLine("expanded", result.expanded ? format(*result.expanded) : "(too long)"),
            };

            if (result.value) {
                lines.push_back(resultLine("value", formatValue(*result.value), theme::accent));
            }
            else if (result.substituted) {
                lines.push_back(resultLine("substituted", format(*result.substituted), theme::accent));
            }
            return vbox(std::move(lines));
        }

        // the variable to plot over: x if the expression has it, otherwise its first variable
        std::string plotVariable() const {
            const std::vector<std::string>& vars = result.assignments.variables;
            if (vars.empty() || std::find(vars.begin(), vars.end(), "x") != vars.end()) {
                return "x";
            }
            return vars[0];
        }

        Element plot() const {

            if (!result.parsed) {
                return hint("nothing to plot");
            }

            std::string var = plotVariable();
            std::shared_ptr<const ::Node> f = result.assignments.apply(*result.simplified, var)->simplify();

            for (const std::string& other : ::variables(*f)) { // the member hides the function
                if (other != var) {
                    return hint("assign " + other + " to plot by " + var);
                }
            }

            // the point of the given value, if the plotted variable has one
            std::optional<PlotPoint> point;
            for (const auto& [name, value] : result.assignments.values) {
                if (name == var && result.value) {
                    point = PlotPoint{value->evaluate({}), *result.value, theme::accent};
                }
            }

            double w = state.plotHalfWidth;
            return plotView({{f, theme::plotFunction}}, var, {-w, w}, point);
        }

        Element render() {
            update();

            Element left = vbox({
                panel("Results", results(), false),
                panel("Variables", variables->view(result.assignments), variables->Focused()),
                panel("History", history->Render() | frame | flex, history->Focused()) | flex,
            });

            Element right;
            if (state.showPlot) {
                right = plot();
            }
            else {
                right = result.parsed ? treeView(*result.parsed) : hint("nothing to show yet");
            }
            Element body = splitBody(left, rightPanel(state, right));

            return vbox({
                expressionBox(input, result.error),
                body | flex,
                statusBar("Tab focus   ↑↓ history   Enter save   F2 tree/plot   PgUp/PgDn zoom   Esc menu"),
            });
        }
    };

}

Component evaluateScreen(TuiState& state) {

    auto screen = std::make_shared<EvaluateScreen>(state);

    std::vector<Component> zones = {screen->input, screen->variables, screen->history};
    Component layout = Renderer(Container::Vertical(zones), [screen] { return screen->render(); });

    return screenKeys(layout, state, zones);
}
