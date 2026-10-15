#include "parser.h"
#include "lexer.h"
#include "errors.h"

#include <stdexcept>

namespace {

    // how a token is named in error messages
    std::string describe(const Token& t) {
        if (t.getType() == TokenType::END) {
            return "end of input";
        }
        return "'" + t.getValue() + "'";
    }

}

/// PUBLIC INTERFACE ///////////////////////////////////////////////////////////////////

std::unique_ptr<Node> parse(const std::string& input) {
    Lexer lexer(input);
    Parser parser(lexer.tokenize());
    return parser.parse();
}

std::unique_ptr<Node> Parser::parse() {

    if (tokens.empty() || tokens.back().getType() != TokenType::END) { // token lists from the Lexer always end with END
        size_t end = tokens.empty() ? 0 : tokens.back().getPosition() + tokens.back().getValue().size();
        tokens.push_back(Token(TokenType::END, "", end));
    }

    std::unique_ptr<Node> result = parseExpression();

    if (peek().getType() != TokenType::END) { // e.g. "x)" or "(x+1))"
        error("Unexpected " + describe(peek()), peek());
    }

    return result;
}

/// GRAMMAR RULES //////////////////////////////////////////////////////////////////////

// expression = term { ("+" | "-") term }
std::unique_ptr<Node> Parser::parseExpression() {

    std::unique_ptr<Node> left = parseTerm();

    while (isOperator('+') || isOperator('-')) {
        BinaryOperator op = advance().getValue() == "+" ? BinaryOperator::ADD : BinaryOperator::SUB;
        left = std::make_unique<BinaryOpNode>(op, std::move(left), parseTerm());
    }

    return left;
}

// term = signed { ("*" | "/") signed | implicit }
std::unique_ptr<Node> Parser::parseTerm() {

    std::unique_ptr<Node> left = parseSigned();

    while (true) {

        if (isOperator('*') || isOperator('/')) {
            BinaryOperator op = advance().getValue() == "*" ? BinaryOperator::MUL : BinaryOperator::DIV;
            left = std::make_unique<BinaryOpNode>(op, std::move(left), parseSigned());
        }
        else if (startsImplicitOperand()) { // 2x, x(x+1), 2sin x
            left = std::make_unique<BinaryOpNode>(BinaryOperator::MUL, std::move(left), parsePower());
        }
        else if (peek().getType() == TokenType::NUMBER) { // "2 3" or "x2" is most likely a typo
            error("Unexpected number " + describe(peek()) + " (write * to multiply)", peek());
        }
        else {
            return left;
        }
    }
}

// signed = ("-" | "+") signed | power
std::unique_ptr<Node> Parser::parseSigned() {

    if (isOperator('-') || isOperator('+')) {
        UnaryOperator op = advance().getValue() == "-" ? UnaryOperator::SUB : UnaryOperator::ADD;
        return std::make_unique<UnaryOpNode>(op, parseSigned());
    }

    return parsePower();
}

// power = primary [ "^" signed ]
// the exponent is parsed with parseSigned(), which calls parsePower() again: 2^3^2 = 2^(3^2)
std::unique_ptr<Node> Parser::parsePower() {

    std::unique_ptr<Node> base = parsePrimary();

    if (isOperator('^')) {
        advance();
        return std::make_unique<BinaryOpNode>(BinaryOperator::POW, std::move(base), parseSigned());
    }

    return base;
}

// primary = NUMBER | VARIABLE | CONSTANT | "(" expression ")" | function
std::unique_ptr<Node> Parser::parsePrimary() {

    const Token& t = peek();

    switch (t.getType()) {

        case TokenType::NUMBER:
            advance();
            try {
                return std::make_unique<NumberNode>(std::stod(t.getValue()));
            }
            catch (const std::out_of_range&) {
                error("Number " + describe(t) + " is too large", t);
            }

        case TokenType::VARIABLE:
            advance();
            return std::make_unique<VarNode>(t.getValue());

        case TokenType::CONSTANT:
            advance();
            return std::make_unique<ConstantNode>(t.getValue(), *constantValue(t.getValue()));

        case TokenType::LPAREN: {
            advance();
            std::unique_ptr<Node> inner = parseExpression();
            expect(TokenType::RPAREN, "')'");
            return inner;
        }

        case TokenType::FUNCTION:
            return parseFunction();

        default: // operator, ')' or end of input where an operand should be
            error("Unexpected " + describe(t), t);
    }
}

// function = FUNCTION [ "^" NUMBER ] ( "(" expression ")" | argument )
std::unique_ptr<Node> Parser::parseFunction() {

    Token function = advance();
    FunctionType type = *stringToFunctionType(function.getValue());

    // sin^2 x = sin(x)^2
    std::unique_ptr<Node> power;
    if (isOperator('^')) {
        advance();

        if (isOperator('-')) { // sin^-1 usually means arcsin, not 1/sin, so refuse to guess
            error("Negative power of '" + function.getValue() + "' is ambiguous: write the inverse function or 1/"
                  + function.getValue() + "(x)", peek());
        }
        if (peek().getType() != TokenType::NUMBER) {
            error("Expected a number after '" + function.getValue() + "^'", peek());
        }

        const Token& exponent = advance();
        double k = std::stod(exponent.getValue());
        if (k == 0.0) {
            error("Power of '" + function.getValue() + "' must be positive", exponent);
        }
        power = std::make_unique<NumberNode>(k);
    }

    // parentheses right after the name are always the argument: sin (x+1)^2 = sin(x+1)^2
    std::unique_ptr<Node> argument;
    if (peek().getType() == TokenType::LPAREN) {
        advance();
        argument = parseExpression();
        expect(TokenType::RPAREN, "')'");
    }
    else {
        argument = parseFunctionArgument(function);
    }

    std::unique_ptr<Node> call = std::make_unique<FuncNode>(type, std::move(argument));

    if (power) {
        return std::make_unique<BinaryOpNode>(BinaryOperator::POW, std::move(call), std::move(power));
    }
    return call;
}

// argument = ("-" | "+") argument | power { implicit }
// the chain stops at an explicit operator or at the next function: sin x cos x = sin(x) * cos(x)
std::unique_ptr<Node> Parser::parseFunctionArgument(const Token& function) {

    if (isOperator('-') || isOperator('+')) { // sin -x = sin(-x)
        UnaryOperator op = advance().getValue() == "-" ? UnaryOperator::SUB : UnaryOperator::ADD;
        return std::make_unique<UnaryOpNode>(op, parseFunctionArgument(function));
    }

    TokenType next = peek().getType();
    if (next == TokenType::END || next == TokenType::OPERATOR || next == TokenType::RPAREN) {
        error("Missing argument of '" + function.getValue() + "'", peek());
    }

    std::unique_ptr<Node> argument = parsePower();

    while (startsImplicitOperand() && peek().getType() != TokenType::FUNCTION) {
        argument = std::make_unique<BinaryOpNode>(BinaryOperator::MUL, std::move(argument), parsePower());
    }

    return argument;
}

/// TOKEN HELPERS //////////////////////////////////////////////////////////////////////

const Token& Parser::peek() const {
    return tokens[current];
}

// never moves past END, so peek() is always valid
const Token& Parser::advance() {
    const Token& t = tokens[current];
    if (t.getType() != TokenType::END) {
        current++;
    }
    return t;
}

bool Parser::isOperator(char op) const {
    return peek().getType() == TokenType::OPERATOR && peek().getValue() == std::string(1, op);
}

// an operand that can follow another one without an operator; numbers are excluded on purpose
bool Parser::startsImplicitOperand() const {
    TokenType t = peek().getType();
    return t == TokenType::VARIABLE || t == TokenType::CONSTANT
        || t == TokenType::FUNCTION || t == TokenType::LPAREN;
}

void Parser::expect(TokenType type, const std::string& what) {
    if (peek().getType() != type) {
        error("Expected " + what + " but found " + describe(peek()), peek());
    }
    advance();
}

void Parser::error(const std::string& message, const Token& at) const {
    throw ParseError(message, at.getPosition());
}
