//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_TOKEN_H
#define MICALANG_TOKEN_H
#include "iostream"
#include "vector"
#include "position.h"

enum TokenKind { TT_STRING, TT_CHAR, TT_DOUBLE, TT_INTEGER, TT_BOOL, TT_NULL, TT_OP, TT_KEY, TT_ID, TT_EOF };

struct Token {
    std::string data;
    TokenKind kind;
    Position begin, end;
    Token();
    Token(std::string, TokenKind, Position , Position );
    void debug() const;
};

bool isKey(const std::string&);

#endif //MICALANG_TOKEN_H
