#include "screens.h"
#include "theme.h"

#include <memory>
#include <string>
#include <vector>

using namespace ftxui;

namespace {

    // In the style of the opencode logo: '_' is a space on the shadow color, '^' a ▀ on the
    // shadow color, '~' a ▀ in the shadow color, so the letters seem to stand out of the screen.
    const std::vector<std::string> LOGO = {
        "              ▄             ▄   ",
        "█▀█▀▄ █▀▀█ ▀█▀▀ █▀▀█ █▀▀▀ █_▄▀",
        "█_█_█ █^^█ _█__ █^^^ █___ █▀▄_",
        "▀~▀~▀ ▀~~▀ _▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀~~▀",
    };

    const std::string TAGLINE = "~ A lightweight symbolic math engine ~";

    // index of the entry that is not implemented yet
    constexpr int COMING_SOON = 2;

    // splits a UTF-8 string into characters: ▀ is three bytes but one cell on screen
    std::vector<std::string> glyphs(const std::string& line) {
        std::vector<std::string> out;
        for (size_t i = 0; i < line.size();) {
            unsigned char lead = line[i];
            size_t length = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
            out.push_back(line.substr(i, length));
            i += length;
        }
        return out;
    }

    Element logo() {
        Elements rows;
        for (const std::string& line : LOGO) {
            Elements cells;
            for (const std::string& g : glyphs(line)) {
                if (g == "_") {
                    cells.push_back(text(" ") | bgcolor(theme::logoShadow));
                }
                else if (g == "^") {
                    cells.push_back(text("▀") | color(theme::logo) | bgcolor(theme::logoShadow));
                }
                else if (g == "~") {
                    cells.push_back(text("▀") | color(theme::logoShadow));
                }
                else {
                    cells.push_back(text(g) | color(theme::logo));
                }
            }
            rows.push_back(hbox(std::move(cells)));
        }
        return vbox(std::move(rows));
    }

}

Component mainMenu(std::function<void(Page)> open) {

    static const std::vector<std::string> entries = {
        "Evaluation and expansion",
        "Symbolic differentiation",
        "Numerical integration (coming soon)",
    };

    auto selected = std::make_shared<int>(0);

    MenuOption option = MenuOption::Vertical();
    option.entries = &entries;
    option.selected = selected.get();
    option.on_enter = [selected, open] {
        if (*selected == 0) {
            open(Page::EVALUATE);
        }
        else if (*selected == 1) {
            open(Page::DIFFERENTIATE);
        }
    };
    option.entries_option.transform = [](const EntryState& entry) {
        Element line = text((entry.active ? "› " : "  ") + entry.label);
        if (entry.index == COMING_SOON) {
            return line | color(theme::muted);
        }
        return entry.active ? line | color(theme::accent) | bold : line;
    };

    Component menu = Menu(option);

    // selected is kept alive by the lambdas above
    return Renderer(menu, [menu] {
        return vbox({
            filler(),
            logo() | hcenter,
            text(""),
            text(TAGLINE) | color(theme::muted) | hcenter,
            text(""),
            text(""),
            menu->Render() | size(HEIGHT, EQUAL, int(entries.size())) | hcenter, // a menu would fill the height
            filler(),
            statusBar("↑↓ select   Enter open   Esc quit"),
        });
    });
}
