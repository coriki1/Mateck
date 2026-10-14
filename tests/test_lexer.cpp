#include "test.h"
#include "lexer.h"
#include "errors.h"

namespace {

    // tokenek tomor szoveges alakja, pl. "F(sin) V(x)"; az END tokent kihagyja
    std::string lex(const std::string& input) {
        std::string result;
        try {
            Lexer lexer(input);
            for (const Token& t : lexer.tokenize()) {
                if (t.getType() == TokenType::END) continue;
                if (!result.empty()) result += " ";
                result += tokenTypeToString(t.getType()).substr(0, 1) + "(" + t.getValue() + ")";
            }
        }
        catch (const LexError&) {
            return "HIBA";
        }
        return result;
    }

    // tokenek pozicioi, pl. "0 4 6"
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

void runLexerTests() {

    // Lexer: ideiglenes stringbol is biztonsagosan dolgozik (korabban logo referencia)
    {
        Lexer lexer(std::string("x+1"));
        checkStr("Lexer ideiglenes stringgel", std::to_string(lexer.tokenize().size()), "4"); // x + 1 END
    }

    // Lexer: fuggvenynev utan kozvetlenul irt valtozo
    checkStr("Lexer sinx", lex("sinx"), "F(sin) V(x)");
    checkStr("Lexer sinhx (leghosszabb egyezes)", lex("sinhx"), "F(sinh) V(x)");
    checkStr("Lexer xsin(y)", lex("xsin(y)"), "V(x) F(sin) L(() V(y) R())");
    checkStr("Lexer 2xy", lex("2xy"), "N(2) V(x) V(y)");

    // Lexer: szamok
    checkStr("Lexer .5", lex(".5"), "N(.5)");
    checkStr("Lexer 1.2.3 hiba", lex("1.2.3"), "HIBA");
    checkStr("Lexer nem ASCII karakter hiba", lex("2*\xCF\x80"), "HIBA");

    // Konstansok
    checkStr("Lexer pi", lex("pi"), "C(pi)");
    checkStr("Lexer e", lex("e"), "C(e)");
    checkStr("Lexer 2pi", lex("2pi"), "N(2) C(pi)");
    checkStr("Lexer pix (leghosszabb egyezes)", lex("pix"), "C(pi) V(x)");
    checkStr("Lexer ex", lex("ex"), "C(e) V(x)");
    checkStr("Lexer 1e5 nem tudomanyos jeloles", lex("1e5"), "N(1) C(e) N(5)");
    checkStr("Lexer sec (nem fuggveny)", lex("sec"), "V(s) C(e) V(c)");

    // END token
    {
        Lexer lexer("x + 1");
        auto tokens = lexer.tokenize();
        checkBool("Lexer utolso token END", tokens.back().getType() == TokenType::END, true);

        Lexer empty("");
        auto emptyTokens = empty.tokenize();
        checkBool("Lexer ures bemenet -> csak END",
            emptyTokens.size() == 1 && emptyTokens[0].getType() == TokenType::END, true);
    }

    // Pozíciok (az END a bemenet hossza)
    checkStr("Lexer poziciok 'sin x + 12'", positions("sin x + 12"), "0 4 6 8 10");
    checkStr("Lexer poziciok '(.5)'", positions("(.5)"), "0 1 3 4");

    // Hibak pozicioval
    {
        try {
            Lexer lexer("x + $");
            lexer.tokenize();
            checkBool("Lexer '$' hibat dob", false, true);
        }
        catch (const LexError& e) {
            check("LexError pozicio", e.getPosition(), 4);
            checkStr("LexError uzenet", e.what(), "Unsupported character '$' at column 5");
        }

        try {
            Lexer lexer("2 * 1.2.3");
            lexer.tokenize();
            checkBool("Lexer 1.2.3 hibat dob", false, true);
        }
        catch (const LexError& e) {
            check("LexError pozicio rossz szamnal", e.getPosition(), 4);
        }

        checkBool("LexError egyben MateckError",
            throwsError<MateckError>([] { Lexer("#").tokenize(); }), true);
    }
}
