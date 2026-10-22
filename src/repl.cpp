#include "repl.h"
#include "parser.h"
#include "errors.h"
#include "format.h"
#include "expand.h"
#include "session.h"

#include <iomanip>
#include <string>
#include <utility>
#include <vector>

namespace {

    constexpr int LABEL_WIDTH = 18; // fits the longest label, "d/dx substituted:"

    void printLine(std::ostream& out, const std::string& label, const std::string& text) {
        out << "  " << std::left << std::setw(LABEL_WIDTH) << label << text << "\n";
    }

    bool isBlank(const std::string& line) {
        return line.find_first_not_of(" \t") == std::string::npos;
    }

    // asks for the value of var until it gets a valid one
    // returns false if the input ended, value stays empty if the answer was empty
    bool askValue(const std::string& var, std::istream& in, std::ostream& out, std::unique_ptr<Node>& value) {

        while (true) {
            out << "  " << var << " = ";

            std::string answer;
            if (!std::getline(in, answer)) {
                out << "\n";
                return false;
            }

            if (isBlank(answer)) { // leave this variable in place
                return true;
            }

            try {
                std::unique_ptr<Node> parsed = parse(answer);

                std::vector<std::string> inner = variables(*parsed);
                if (!inner.empty()) {
                    out << "  Error: the value cannot contain a variable (" << inner[0] << ")\n";
                    continue;
                }

                value = std::move(parsed);
                return true;
            }
            catch (const MateckError& e) {
                out << "  Error: " << e.what() << "\n";
            }
        }
    }

    // handles one expression; returns false if the input ended while asking for values
    bool processExpression(const std::string& line, std::istream& in, std::ostream& out) {

        std::unique_ptr<Node> expr;
        try {
            expr = parse(line);
        }
        catch (const MateckError& e) {
            out << "  Error: " << e.what() << "\n";
            return true;
        }

        printLine(out, "parsed:", format(*expr));
        printLine(out, "simplified:", format(*expr->simplify()));

        std::unique_ptr<Node> expanded = expand(*expr);
        printLine(out, "expanded:", expanded ? format(*expanded) : "(too long)");

        std::unique_ptr<Node> derivative; // stays empty for e.g. floor(x)
        try {
            derivative = expr->differentiate("x");
            printLine(out, "d/dx:", format(*derivative->simplify()));
        }
        catch (const DiffError& e) {
            printLine(out, "d/dx:", e.what());
        }

        // ask for the variables in order of first appearance
        std::vector<std::string> vars = variables(*expr);
        std::vector<std::pair<std::string, std::unique_ptr<Node>>> values;

        for (const std::string& var : vars) {
            std::unique_ptr<Node> value;
            if (!askValue(var, in, out, value)) {
                return false;
            }
            if (value) {
                values.emplace_back(var, std::move(value));
            }
        }

        if (!vars.empty() && values.empty()) { // every answer was empty
            return true;
        }

        for (const auto& [var, value] : values) {
            expr = expr->substitute(var, *value);
            if (derivative) {
                derivative = derivative->substitute(var, *value);
            }
        }

        if (values.size() == vars.size()) { // nothing left to substitute: show numbers
            printLine(out, "value:", formatValue(expr->evaluate({})));
            if (derivative) {
                printLine(out, "d/dx value:", formatValue(derivative->evaluate({})));
            }
        }
        else { // some variables stay: show the substituted expression
            printLine(out, "substituted:", format(*expr->simplify()));
            if (derivative) {
                printLine(out, "d/dx substituted:", format(*derivative->simplify()));
            }
        }

        return true;
    }

}

void runInteractive(std::istream& in, std::ostream& out) {

    std::string line;
    out << "> ";

    while (std::getline(in, line)) {

        if (!isBlank(line) && !processExpression(line, in, out)) {
            return; // input ended while asking for a value
        }

        out << "> ";
    }

    out << "\n";
}
