#pragma once

#include <iostream>

// Interactive mode: reads expressions line by line from in and writes to out
// the parsed form, the simplified form and the derivative by x. Then it asks for
// the value of every variable; an empty answer leaves that variable in place.
// Returns when the input ends.
void runInteractive(std::istream& in, std::ostream& out);
