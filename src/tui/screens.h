#pragma once

#include "panels.h"

#include <ftxui/component/component.hpp>

#include <functional>

// the pages of the terminal UI, in the order of the main menu
enum class Page { MENU, EVALUATE, DIFFERENTIATE };

ftxui::Component mainMenu(std::function<void(Page)> open);
ftxui::Component evaluateScreen(TuiState& state);
ftxui::Component differentiateScreen(TuiState& state);
