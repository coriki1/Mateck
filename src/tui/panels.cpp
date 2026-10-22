#include "panels.h"
#include "theme.h"
#include "tree.h"

#include <ftxui/component/event.hpp>
#include <ftxui/screen/terminal.hpp>

#include <algorithm>

using namespace ftxui;

namespace {

    constexpr const char* VERSION = "v0.1";

    constexpr int LABEL_WIDTH = 15;         // fits the longest label, "d³/dx³ value"
    constexpr int MIN_WIDE_TERMINAL = 80;   // narrower terminals hide the right panel

    constexpr double MIN_PLOT_HALF_WIDTH = 1e-3;
    constexpr double MAX_PLOT_HALF_WIDTH = 1e6;

    // inputs show their placeholder dimmed and nothing else: the focus shows on the panel border
    // a rounded frame in the given color with the title on its top edge; unlike window() it
    // leaves the content's colors alone (an uncolored text would take the border color)
    Element framed(Element title, Element content, Color border) {
        return dbox({
            content | borderStyled(BorderStyle::ROUNDED, border),
            vbox({hbox({text("╭") | color(border), std::move(title)}), filler()}), // top row only
        });
    }

    InputOption plainInput() {
        InputOption option;
        option.multiline = false;
        option.transform = [](InputState s) {
            if (s.is_placeholder) {
                s.element |= color(theme::muted);
            }
            return s.element;
        };
        return option;
    }

    void showExpression(TuiState& state, const std::string& text) {
        state.expression = text;
        state.cursor = static_cast<int>(text.size());
    }

    void remember(TuiState& state) {

        state.historyBrowsed = -1;

        if (state.expression.find_first_not_of(" \t") == std::string::npos) {
            return;
        }
        if (!state.history.empty() && state.history.front() == state.expression) {
            return;
        }

        state.history.insert(state.history.begin(), state.expression);
        if (state.history.size() > MAX_HISTORY) {
            state.history.pop_back();
        }
        state.historySelected = 0;
    }

    // up goes to older entries, down back to newer ones and finally to the draft
    bool browseHistory(TuiState& state, int step) {

        int target = state.historyBrowsed + step;
        if (target < -1 || target >= static_cast<int>(state.history.size())) {
            return false;
        }

        if (state.historyBrowsed == -1) {
            state.draft = state.expression;
        }
        state.historyBrowsed = target;
        showExpression(state, target == -1 ? state.draft : state.history[target]);
        return true;
    }

}

/// COMPONENTS ///

Component expressionInput(TuiState& state) {

    InputOption option = plainInput();
    option.content = &state.expression;
    option.placeholder = "type an expression, e.g. x^3 - 2x";
    option.cursor_position = &state.cursor;
    option.on_enter = [&state] { remember(state); };
    option.on_change = [&state] { state.historyBrowsed = -1; };

    return Input(option) | CatchEvent([&state](Event event) {
        if (event == Event::ArrowUp) {
            browseHistory(state, +1);
            return true;
        }
        if (event == Event::ArrowDown) {
            browseHistory(state, -1);
            return true;
        }
        return false;
    });
}

Component historyMenu(TuiState& state, std::function<void()> onPick) {

    MenuOption option = MenuOption::Vertical();
    option.entries = &state.history;
    option.selected = &state.historySelected;
    option.on_enter = [&state, onPick] {
        if (state.historySelected < static_cast<int>(state.history.size())) {
            showExpression(state, state.history[state.historySelected]);
            state.historyBrowsed = -1;
            onPick();
        }
    };
    option.entries_option.transform = [](const EntryState& entry) {
        Element line = text((entry.active ? "> " : "  ") + entry.label);
        if (entry.focused) {
            line |= color(theme::accent);
        }
        return line;
    };

    return Menu(option);
}

void VariablesPanel::sync(const std::vector<std::string>& variables) {

    if (variables == names) {
        return;
    }
    names = variables;

    DetachAllChildren();
    inputs.clear();

    Component column = Container::Vertical({});
    for (const std::string& name : names) {
        InputOption option = plainInput();
        option.content = &state.values[name];
        option.placeholder = "no value";
        inputs.push_back(Input(option));
        column->Add(inputs.back());
    }
    Add(column);
}

Element VariablesPanel::view(const Assignments& assignments) {

    if (names.empty()) {
        return hint("no variables");
    }

    Elements rows;
    for (size_t i = 0; i < names.size(); ++i) {
        rows.push_back(hbox({text(names[i] + " = "), inputs[i]->Render() | flex}));

        // on its own line: beside the input it would squeeze the input to nothing
        auto error = assignments.errors.find(names[i]);
        if (error != assignments.errors.end()) {
            rows.push_back(paragraphAlignLeft("    " + error->second) | color(theme::error));
        }
    }
    return vbox(std::move(rows));
}

Component screenKeys(Component screen, TuiState& state, std::vector<Component> zones) {

    return CatchEvent(screen, [&state, zones](Event event) {

        if (event == Event::Tab || event == Event::TabReverse) {
            int n = static_cast<int>(zones.size());
            int step = event == Event::Tab ? 1 : n - 1;

            int current = 0;
            for (int i = 0; i < n; ++i) {
                if (zones[i]->Focused()) {
                    current = i;
                }
            }

            // the next zone that can take the focus (an empty variables panel cannot)
            for (int i = (current + step) % n; i != current; i = (i + step) % n) {
                if (zones[i]->Focusable()) {
                    zones[i]->TakeFocus();
                    break;
                }
            }
            return true;
        }

        if (event == Event::F2) {
            state.showPlot = !state.showPlot;
            return true;
        }
        if (event == Event::PageUp) {
            state.plotHalfWidth = std::max(state.plotHalfWidth / 2, MIN_PLOT_HALF_WIDTH);
            return true;
        }
        if (event == Event::PageDown) {
            state.plotHalfWidth = std::min(state.plotHalfWidth * 2, MAX_PLOT_HALF_WIDTH);
            return true;
        }
        return false;
    });
}

/// ELEMENTS ///

Element panel(const std::string& title, Element content, bool focused) {
    Color border = focused ? theme::accent : theme::muted;
    return framed(text(" " + title + " ") | bold | color(border), std::move(content), border);
}

Element expressionBox(const Component& input, const std::optional<InputError>& error) {

    Elements lines = {hbox({text("> ") | color(theme::accent), input->Render() | flex})};

    if (error) {
        if (error->column) { // the input starts after "> "
            lines.push_back(text(std::string(*error->column + 2, ' ') + "^") | color(theme::error));
        }
        lines.push_back(text(error->message) | color(theme::error));
    }

    return panel("Expression", vbox(std::move(lines)), input->Focused());
}

Element resultLine(const std::string& label, const std::string& value, Color valueColor) {
    return hbox({
        text(label) | color(theme::label) | size(WIDTH, EQUAL, LABEL_WIDTH),
        paragraphAlignLeft(value) | color(valueColor) | flex,
    });
}

Element hint(const std::string& message) {
    return text(message) | color(theme::muted);
}

Element rightPanel(const TuiState& state, Element content) {

    auto tab = [](const std::string& name, bool open) {
        Element t = text(" " + name + " ");
        return open ? t | color(theme::accent) | bold | underlined : t | color(theme::muted);
    };

    // separated by a space: a │ in the title would join the border of the panel above
    Element tabs = hbox({tab("Tree", !state.showPlot), text(" "), tab("Plot", state.showPlot)});

    return framed(tabs, content | flex, theme::muted);
}

Element treeView(const ::Node& node) {
    Elements lines;
    for (const std::string& line : drawTree(node)) {
        lines.push_back(text(line));
    }
    return vbox(std::move(lines)) | frame | flex;
}

Element statusBar(const std::string& keys) {
    // xframe cuts the keys off in a narrow terminal instead of writing over the version
    return hbox({text(" " + keys) | xframe | flex, text(" " + std::string(VERSION) + " ")})
           | color(theme::muted);
}

Element splitBody(Element left, Element right) {
    int width = Terminal::Size().dimx;
    if (width < MIN_WIDE_TERMINAL) {
        return left | flex;
    }
    return hbox({left | size(WIDTH, EQUAL, width / 2), right | flex});
}
