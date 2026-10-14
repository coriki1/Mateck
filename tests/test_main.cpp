#include "test.h"

#include <cmath>
#include <iostream>
#include <sstream>

namespace {
    int passed = 0;
    int total = 0;

    std::string format(double v) {
        std::ostringstream out;
        out << v;
        return out.str();
    }

    void report(const std::string& label, bool ok, const std::string& expected, const std::string& actual) {
        total++;
        if (ok) passed++;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << label
                  << " (elvart: " << expected << ", kapott: " << actual << ")\n";
    }
}

void check(const std::string& label, double actual, double expected, double eps) {
    report(label, std::fabs(actual - expected) < eps, format(expected), format(actual));
}

void checkBool(const std::string& label, bool actual, bool expected) {
    report(label, actual == expected, expected ? "true" : "false", actual ? "true" : "false");
}

void checkStr(const std::string& label, const std::string& actual, const std::string& expected) {
    report(label, actual == expected, expected, actual);
}

int main() {

    std::cout << "--- Lexer ---\n";
    runLexerTests();

    std::cout << "\n--- Nodes ---\n";
    runNodeTests();

    std::cout << "\n--- Simplify ---\n";
    runSimplifyTests();

    std::cout << "\n" << passed << "/" << total << " teszt sikeres.\n";

    return (passed == total) ? 0 : 1;
}
