#pragma once

#include <string>

/// SHARED TEST HELPERS ///

void check(const std::string& label, double actual, double expected, double eps = 1e-9);
void checkBool(const std::string& label, bool actual, bool expected);
void checkStr(const std::string& label, const std::string& actual, const std::string& expected);

// true if calling f throws an exception of type E
template <typename E, typename F>
bool throwsError(F f) {
    try {
        f();
    }
    catch (const E&) {
        return true;
    }
    catch (...) {
        return false;
    }
    return false;
}

/// TEST GROUPS (one per file) ///

void runLexerTests();
void runNodeTests();
void runSimplifyTests();
