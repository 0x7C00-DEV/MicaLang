//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_LEXER_H
#define MICALANG_LEXER_H
#include "token.h"

enum {
    SYN_TYPE,
    SYN_VALUE,
    SYN_CALL,
};

class Lexer {
public:
    explicit Lexer(std::string);
    Lexer();

    Token getToken(int);

    void resetExpr(std::string);
    void saveState();
    void restore();
private:
    std::string expr;
    int pos;
    char current;

    struct State {
        std::string expr;
        int pos;
        char current;
    };

    std::vector<State> states;

    int lin, col;
    void advance(int);

    [[nodiscard]] bool fcmp(const std::string&) const;

    Token getString();
    Token getNumber();
    Token getIdOrKey();
    Token getSymbol(int);
    Token getChar();
};

#endif //MICALANG_LEXER_H
