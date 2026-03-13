#include "include/token.h"
#include <string>
#include <iostream>

int main() {
    std::string testexp = "3.14 + 2*(1 - 5.5) / (7 - 3) + 42";
    Lexer obj;
    std::vector<Token> TokenArr = obj.Tokenize(testexp);

    for(int i = 0; i<TokenArr.size(); i++) {
        TokenArr[i].print();
    }
}