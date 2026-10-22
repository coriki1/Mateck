#include "plot.h"
#include "theme.h"

#include <ftxui/dom/canvas.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace ftxui;

namespace {

    // samples used to choose the y range, independent of the canvas size
    constexpr int RANGE_SAMPLES = 200;

    constexpr int POINT_RADIUS = 1;

    std::string shortNumber(double v) {
        std::ostringstream out;
        out << std::setprecision(4) << v;
        return out.str();
    }

    // canvas row of value v; the canvas y axis points down
    double row(double v, Range y, int height) {
        return (y.max - v) / (y.max - y.min) * (height - 1);
    }

    double column(double v, Range x, int width) {
        return (v - x.min) / (x.max - x.min) * (width - 1);
    }

    void drawCurve(Canvas& c, const Curve& curve, const std::string& var, Range x, Range y) {

        int w = c.width();
        int h = c.height();
        std::vector<double> samples = samplePlot(*curve.f, var, x, w);

        for (int i = 0; i + 1 < w; ++i) {
            if (!std::isfinite(samples[i]) || !std::isfinite(samples[i + 1])) {
                continue;
            }

            double a = row(samples[i], y, h);
            double b = row(samples[i + 1], y, h);

            // a jump across the whole plot is a pole (tan x, 1/x), not a steep curve
            if (std::abs(a - b) > h) {
                continue;
            }
            if ((a < 0 && b < 0) || (a >= h && b >= h)) { // both outside the view
                continue;
            }

            // keeps the line short where a point lies far outside
            a = std::clamp(a, -1.0, double(h));
            b = std::clamp(b, -1.0, double(h));
            c.DrawPointLine(i, int(std::lround(a)), i + 1, int(std::lround(b)), curve.color);
        }
    }

}

Element plotView(const std::vector<Curve>& curves, const std::string& var, Range xRange,
                 std::optional<PlotPoint> point) {

    std::vector<double> rangeSamples;
    for (const Curve& curve : curves) {
        if (curve.setsRange) {
            std::vector<double> s = samplePlot(*curve.f, var, xRange, RANGE_SAMPLES);
            rangeSamples.insert(rangeSamples.end(), s.begin(), s.end());
        }
    }

    std::optional<Range> yRange = plotRange(rangeSamples);
    if (!yRange) {
        return text("undefined on " + var + " = " + shortNumber(xRange.min) + " .. "
                    + shortNumber(xRange.max)) | color(theme::muted);
    }
    Range y = *yRange;

    Element plot = canvas([=](Canvas& c) {
        int w = c.width();
        int h = c.height();
        if (w < 2 || h < 2) {
            return;
        }

        if (y.min <= 0 && 0 <= y.max) {
            int r = int(std::lround(row(0, y, h)));
            c.DrawPointLine(0, r, w - 1, r, theme::plotAxis);
        }
        if (xRange.min <= 0 && 0 <= xRange.max) {
            int col = int(std::lround(column(0, xRange, w)));
            c.DrawPointLine(col, 0, col, h - 1, theme::plotAxis);
        }

        for (const Curve& curve : curves) {
            drawCurve(c, curve, var, xRange, y);
        }

        if (point && std::isfinite(point->y)) {
            c.DrawPointCircleFilled(int(std::lround(column(point->x, xRange, w))),
                                    int(std::lround(row(point->y, y, h))), POINT_RADIUS, point->color);
        }
    });

    Element ranges = text(var + ": " + shortNumber(xRange.min) + " .. " + shortNumber(xRange.max)
                          + "   y: " + shortNumber(y.min) + " .. " + shortNumber(y.max))
                     | color(theme::muted);

    return vbox({plot | flex, ranges});
}
