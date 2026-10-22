#include "repl.h"
#include "tui.h"

#include <iostream>
#include <string>
#include <unistd.h>

namespace {

    void printUsage(std::ostream& out) {
        out << "Usage: mateck [--plain]\n"
               "\n"
               "  (no option)  full-screen terminal UI\n"
               "  --plain      line-based interactive mode, also used when the input or output\n"
               "               is not a terminal, e.g. echo \"x^2\" | mateck\n"
               "  -h, --help   show this help\n";
    }

}

int main(int argc, char* argv[]) {

    bool plain = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--plain") {
            plain = true;
        }
        else if (arg == "-h" || arg == "--help") {
            printUsage(std::cout);
            return 0;
        }
        else {
            std::cerr << "mateck: unknown option '" << arg << "'\n\n";
            printUsage(std::cerr);
            return 2;
        }
    }

    // the UI needs a terminal on both ends; piped input gets the line-based mode
    if (plain || !isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        runInteractive(std::cin, std::cout);
    }
    else {
        runTui();
    }
    return 0;
}
