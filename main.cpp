#include "repl.h"

#include <iostream>

// Interactive mode, see include/repl.h
// The tests are in tests/, run them with: make test
int main() {
    runInteractive(std::cin, std::cout);
    return 0;
}
