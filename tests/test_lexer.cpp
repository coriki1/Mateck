#include <gtest/gtest.h>

#include "lexer.h"
#include "errors.h"

namespace {

    // compact form of the tokens, e.g. "F(sin) V(x)"; the END token is left out
    std::string lex(const std::string& input) {
        std::string result;
        Lexer lexer(input);
        for (const Token& t : lexer.tokenize()) {
            if (t.getType() == TokenType::END) continue;
            if (!result.empty()) result += " ";
            result += tokenTypeToString(t.getType()).substr(0, 1) + "(" + t.getValue() + ")";
        }
        return result;
    }

    // token positions, e.g. "0 4 6"
    std::string positions(const std::string& input) {
        std::string result;
        Lexer lexer(input);
        for (const Token& t : lexer.tokenize()) {
            if (!result.empty()) result += " ";
            result += std::to_string(t.getPosition());
        }
        return result;
    }

}

// used to keep a dangling reference to the input
TEST(Lexer, OwnsTemporaryInput) {
    Lexer lexer(std::string("x+1"));
    EXPECT_EQ(lexer.tokenize().size(), 4u); // x + 1 END
}

TEST(Lexer, SplitsFunctionFromFollowingVariable) {
    EXPECT_EQ(lex("sinx"), "F(sin) V(x)");
    EXPECT_EQ(lex("sinhx"), "F(sinh) V(x)");    // longest match wins
    EXPECT_EQ(lex("xsin(y)"), "V(x) F(sin) L(() V(y) R())");
    EXPECT_EQ(lex("2xy"), "N(2) V(x) V(y)");
}

TEST(Lexer, Numbers) {
    EXPECT_EQ(lex(".5"), "N(.5)");
    EXPECT_THROW(lex("1.2.3"), LexError);
    EXPECT_THROW(lex("2*\xCF\x80"), LexError);  // non-ASCII character
}

TEST(Lexer, Constants) {
    EXPECT_EQ(lex("pi"), "C(pi)");
    EXPECT_EQ(lex("e"), "C(e)");
    EXPECT_EQ(lex("2pi"), "N(2) C(pi)");
    EXPECT_EQ(lex("pix"), "C(pi) V(x)");        // longest match wins
    EXPECT_EQ(lex("ex"), "C(e) V(x)");
    EXPECT_EQ(lex("1e5"), "N(1) C(e) N(5)");    // no scientific notation
    EXPECT_EQ(lex("sec"), "V(s) C(e) V(c)");    // sec is not a function
}

TEST(Lexer, EndsWithEndToken) {
    Lexer lexer("x + 1");
    EXPECT_EQ(lexer.tokenize().back().getType(), TokenType::END);

    Lexer empty("");
    auto tokens = empty.tokenize();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].getType(), TokenType::END);
}

// END is positioned at the length of the input
TEST(Lexer, Positions) {
    EXPECT_EQ(positions("sin x + 12"), "0 4 6 8 10");
    EXPECT_EQ(positions("(.5)"), "0 1 3 4");
}

TEST(Lexer, UnsupportedCharacterErrorHasPosition) {
    try {
        Lexer("x + $").tokenize();
        FAIL() << "expected LexError";
    }
    catch (const LexError& e) {
        EXPECT_EQ(e.getPosition(), 4u);
        EXPECT_STREQ(e.what(), "Unsupported character '$' at column 5");
    }
}

TEST(Lexer, MalformedNumberErrorPointsToNumberStart) {
    try {
        Lexer("2 * 1.2.3").tokenize();
        FAIL() << "expected LexError";
    }
    catch (const LexError& e) {
        EXPECT_EQ(e.getPosition(), 4u);
    }
}

TEST(Lexer, LexErrorIsMateckError) {
    EXPECT_THROW(Lexer("#").tokenize(), MateckError);
}
