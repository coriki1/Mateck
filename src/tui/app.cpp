#include "tui.h"
#include "screens.h"

#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

using namespace ftxui;

void runTui() {

    App app = App::Fullscreen();
    TuiState state;
    int page = static_cast<int>(Page::MENU);

    Component evaluate = evaluateScreen(state);
    Component differentiate = differentiateScreen(state);
    Component menu = mainMenu([&page](Page p) { page = static_cast<int>(p); });

    // the order matches the Page enum
    Component pages = Container::Tab({menu, evaluate, differentiate}, &page);

    Component root = CatchEvent(pages, [&](Event event) {
        if (event == Event::Escape) {
            if (page == static_cast<int>(Page::MENU)) {
                app.Exit();
            }
            page = static_cast<int>(Page::MENU);
            return true;
        }
        return false;
    });

    app.Loop(root);
}
