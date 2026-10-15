#include "parser.h"
#include "errors.h"

#include <iostream>
#include <string>

// Interaktiv mod: beolvas egy kifejezest, kiirja ahogy ertelmezte,
// az egyszerusitett alakjat, az erteket (ha nincs benne valtozo) es az x szerinti derivaltjat.
// A tesztek a tests/ mappaban vannak, futtatas: make test
int main() {

    std::string line;
    std::cout << "> ";

    while (std::getline(std::cin, line)) {

        if (line.find_first_not_of(" \t") == std::string::npos) { // empty line
            std::cout << "> ";
            continue;
        }

        try {
            std::unique_ptr<Node> expr = parse(line);

            std::cout << "  ertelmezes:    " << expr->toString() << "\n";
            std::cout << "  egyszerusitve: " << expr->simplify()->toString() << "\n";

            try {
                double value = expr->evaluate({}); // evaluate first, so nothing is printed if it fails
                std::cout << "  ertek:         " << value << "\n";
            }
            catch (const EvalError&) {
                // contains a variable, no value to show
            }

            try {
                std::string derivative = expr->differentiate("x")->simplify()->toString();
                std::cout << "  d/dx:          " << derivative << "\n";
            }
            catch (const DiffError& e) {
                std::cout << "  d/dx:          " << e.what() << "\n";
            }
        }
        catch (const MateckError& e) {
            std::cout << "  Hiba: " << e.what() << "\n";
        }

        std::cout << "> ";
    }

    return 0;
}
