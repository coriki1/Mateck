#include "repl.h"

#include <iostream>

// Interaktiv mod, lasd: include/repl.h
// A tesztek a tests/ mappaban vannak, futtatas: make test
int main() {
    runInteractive(std::cin, std::cout);
    return 0;
}
