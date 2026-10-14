#pragma once

#include <stdexcept>
#include <string>

/// BASE CLASS : MATECKERROR ///////////////////////////////////////////////////////////
// every error Mateck reports about the user's input derives from this

class MateckError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// POSITIONED ERRORS //////////////////////////////////////////////////////////////////
// errors that point to a place in the input string, e.g. "Unexpected ')' at column 7"

class PositionedError : public MateckError {
    size_t position; // 0-based index into the input
public:
    PositionedError(const std::string& message, size_t pos)
        : MateckError(message + " at column " + std::to_string(pos + 1)), position(pos) {}

    size_t getPosition() const { return position; }
};

class LexError : public PositionedError {      // unsupported character, malformed number
public:
    using PositionedError::PositionedError;
};

class ParseError : public PositionedError {    // unexpected token, missing ')'
public:
    using PositionedError::PositionedError;
};

/// OTHER ERRORS ///////////////////////////////////////////////////////////////////////

class EvalError : public MateckError {         // undefined variable
public:
    using MateckError::MateckError;
};

class DiffError : public MateckError {         // non-differentiable function, e.g. floor
public:
    using MateckError::MateckError;
};
