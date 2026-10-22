#pragma once

#include "session.h"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

/// SHARED STATE ////////////////////////////////////////////////////////////////////////
// what the user typed, shared by the screens so it stays when they switch between them

struct TuiState {
    std::string expression;
    int cursor = 0;                    // cursor position in the expression input
    std::vector<std::string> history;  // newest first
    int historySelected = 0;
    int historyBrowsed = -1;           // entry recalled with the arrow keys, -1 for none
    std::string draft;                 // what was typed before browsing the history
    ValueInputs values;                // kept per name, also while the variable is not in use
    bool showPlot = false;             // right panel: tree or plot
    double plotHalfWidth = 10;         // the plot shows [-plotHalfWidth, plotHalfWidth]
};

constexpr size_t MAX_HISTORY = 50;

/// COMPONENTS ///

// one-line input for the expression: Enter saves it to the history, the arrow keys recall it
ftxui::Component expressionInput(TuiState& state);

// list of earlier expressions; Enter puts the selected one back into the input
ftxui::Component historyMenu(TuiState& state, std::function<void()> onPick);

// one input per variable of the current expression, rebuilt when the variables change
class VariablesPanel : public ftxui::ComponentBase {
public:
    explicit VariablesPanel(TuiState& state) : state(state) {}

    void sync(const std::vector<std::string>& variables);
    ftxui::Element view(const Assignments& assignments);

private:
    TuiState& state;
    std::vector<std::string> names;
    std::vector<ftxui::Component> inputs;
};

// the keys every screen shares: Tab and Shift+Tab move between the zones,
// F2 switches the right panel, PageUp and PageDown zoom the plot
ftxui::Component screenKeys(ftxui::Component screen, TuiState& state,
                            std::vector<ftxui::Component> zones);

/// ELEMENTS ///

// a framed panel; its border is highlighted while it has the focus
ftxui::Element panel(const std::string& title, ftxui::Element content, bool focused);

// the expression input in its panel, with the error and a ^ under its column if there is one
ftxui::Element expressionBox(const ftxui::Component& input, const std::optional<InputError>& error);

// "label   value" with the value wrapped to the panel width
ftxui::Element resultLine(const std::string& label, const std::string& value,
                          ftxui::Color valueColor = ftxui::Color::Default);

ftxui::Element hint(const std::string& text);

// the tree / plot panel on the right, with the tab line showing which one is open
ftxui::Element rightPanel(const TuiState& state, ftxui::Element content);

ftxui::Element treeView(const Node& node);

ftxui::Element statusBar(const std::string& keys);

// the left and the right panel side by side in equal halves; a terminal narrower than
// 80 columns shows only the left one
ftxui::Element splitBody(ftxui::Element left, ftxui::Element right);
