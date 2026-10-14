#include "lexer.h"
#include "errors.h"

#include <iostream>
#include <string>

// Ideiglenes: amig nincs parser, a beirt sort tokenekre bontja.
// A tesztek a tests/ mappaban vannak, futtatas: make test
int main() {

    std::string line;
    std::cout << "> ";

    while (std::getline(std::cin, line)) {
        try {
            Lexer lexer(line);
            for (const Token& t : lexer.tokenize()) {
                t.print();
            }
        }
        catch (const MateckError& e) {
            std::cout << "Hiba: " << e.what() << "\n";
        }
        std::cout << "> ";
    }

    return 0;
}
