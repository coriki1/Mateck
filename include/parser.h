#pragma once

#include "node.h"
#include "token.h"

#include <memory>
#include <string>
#include <vector>

/// PARSER ///////////////////////////////////////////////////////////////////////////////
//
// Recursive descent parser, one function per grammar rule (lowest precedence first):
//
//   expression = term { ("+" | "-") term }
//   term       = signed { ("*" | "/") signed | implicit }    implicit multiplication binds like *
//   signed     = ("-" | "+") signed | power
//   power      = primary [ "^" signed ]                       right-associative, -x^2 = -(x^2)
//   primary    = NUMBER | VARIABLE | CONSTANT | "(" expression ")" | function
//   function   = FUNCTION [ "^" NUMBER ] ( "(" expression ")" | argument )
//   argument   = ("-" | "+") argument | power { implicit }    sin x^2 = sin(x^2), ends at + - * / or a function
//
// "implicit" is an operand that directly follows another one without an operator: 2x, x(x+1), 2sin x.
// It may not start with a number, so "2 3" and "x2" are errors.

class Parser {
    std::vector<Token> tokens;
    size_t current;
public:

    explicit Parser(std::vector<Token> t) : tokens(std::move(t)), current(0) {}

    // parses the whole token list, throws ParseError on invalid input
    std::unique_ptr<Node> parse();

private:

    std::unique_ptr<Node> parseExpression();
    std::unique_ptr<Node> parseTerm();
    std::unique_ptr<Node> parseSigned();
    std::unique_ptr<Node> parsePower();
    std::unique_ptr<Node> parsePrimary();
    std::unique_ptr<Node> parseFunction();
    std::unique_ptr<Node> parseFunctionArgument(const Token& function);

    // token helpers
    const Token& peek() const;
    const Token& advance();
    bool isOperator(char op) const;
    bool startsImplicitOperand() const;
    void expect(TokenType type, const std::string& what);

    [[noreturn]] void error(const std::string& message, const Token& at) const;
};

// lexes and parses an expression string, e.g. parse("sin x^2 + 1")
std::unique_ptr<Node> parse(const std::string& input);
