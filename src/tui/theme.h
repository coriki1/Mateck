#pragma once

#include <ftxui/screen/color.hpp>

// Colors of the terminal UI by role. They are the terminal's own 16 palette colors, so the
// terminal theme decides the exact shades, and the background is never painted, so a
// transparent terminal stays transparent.
namespace theme {

    using ftxui::Color;

    inline const Color muted      = Color::GrayDark;  // borders, hints, disabled entries
    inline const Color accent     = Color::Blue;      // focused panel, selected entry
    inline const Color label      = Color::Cyan;      // result labels: parsed, value, ...
    inline const Color derivative = Color::Green;     // derivatives
    inline const Color error      = Color::Red;

    inline const Color plotFunction   = Color::Blue;
    inline const Color plotDerivative = Color::Magenta;
    inline const Color plotTangent    = Color::Yellow;
    inline const Color plotAxis       = Color::GrayDark;

    // the logo uses fixed greys from the 256-color range for its shadow:
    // palette color 8 would be too bright for it
    inline const Color logo       = Color::White;
    inline const Color logoShadow = Color::Grey23;

}
