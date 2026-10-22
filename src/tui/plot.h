#pragma once

#include "node.h"
#include "session.h"

#include <ftxui/dom/elements.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Curve {
    std::shared_ptr<const Node> f;  // shared: the plot is drawn after the screen built it
    ftxui::Color color;
    bool setsRange = true;          // false for the tangent, which would stretch the y range
};

struct PlotPoint {
    double x, y;
    ftxui::Color color;
};

// The curves as functions of var over xRange, drawn with braille dots. The y range fits the
// curves that set it; the axes are drawn where 0 is in view, the ranges are printed below.
ftxui::Element plotView(const std::vector<Curve>& curves, const std::string& var, Range xRange,
                        std::optional<PlotPoint> point);
