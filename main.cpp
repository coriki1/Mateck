#include <iostream>
#include "lexer.h"

int main() {
    std::vector<std::string> tests = {
        "3.14",
        "sin(x)",
        "2x + 1",
        "x^2 + 2*x + 1",
        "-x",
        "sin(cos(x))",
        "log(x) + ln(x)",
        "2(x+1)",
        "1.2.3",      // hiba
        "2 @ 3",      // hiba
    };

    for (const auto& input : tests) {
        std::cout << "Input: " << input << "\n";
        try {
            Lexer lexer(input);
            auto tokens = lexer.tokenize();
            for (const auto& tok : tokens) {
                tok.print();
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Hiba: " << e.what() << "\n";
        }
        std::cout << "---\n";
    }

    return 0;
}