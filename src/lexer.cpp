//
// Created by Lenovo on 2026/9/11.
//
#include <utility>

#include "../include/lexer.h"

void Lexer::advance(int step = 1) {
    for (int i = 0; i < step; ++i) {
        pos++;
        if (pos < (int)expr.size() && (expr[pos] == '\n' || expr[pos] == '\r')) {
            ++lin;
            col = 1;
        } else {
            ++col;
        }
    }
    current = (pos < (int)expr.size()) ? expr[pos] : 0;
}

Lexer::Lexer() {
    pos = -1;
    current = 0;
    lin = col = 1;
}

static bool isWord(const char c) {
    return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || c == '_';
}

Lexer::Lexer(std::string expr) {
    this->expr = std::move(expr);
    this->pos = -1;
    this->current = 0;
    lin = col = 1;
    advance();
}

void Lexer::resetExpr(std::string expression) {
    this->expr = std::move(expression);
    pos = -1;
    current = 0;
    advance();
}

Token Lexer::getToken(const int syntax) {
    BEGIN:
    if (fcmp("//"))
        while (current && current != '\n' && current != '\r')
            advance();
    if (!current) return {"", TT_EOF, lin, col};
    if (current == '\'') return getChar();
    if (current == '"') return getString();
    if (std::isdigit(current)) return getNumber();
    if (isWord(current)) return getIdOrKey();
    if (!std::isspace(current)) return getSymbol(syntax);
    while (std::isspace(current)) {
        if (!current) return {"", TT_EOF, lin, col};
        advance();
    }
    goto BEGIN;
}

void Lexer::saveState() {
    State s;
    s.current = current;
    s.pos = pos;
    s.expr = expr;
    states.push_back(s);
}

void Lexer::restore() {
    if (states.empty()) return;
    auto tmp = states.back();
    states.pop_back();
    current = tmp.current;
    pos = tmp.pos;
    expr = tmp.expr;
}

Token Lexer::getChar() {
    advance();
    std::string res;
    res += current;
    advance();
    advance();
    return {res, TT_CHAR, lin, col};
}

bool Lexer::fcmp(const std::string& name) const {
    if (pos + name.size() > expr.size())
        return false;
    for (int i=0; i<name.size(); ++i)
        if (expr[pos+i] != name[i])
            return false;
    return true;
}

Token Lexer::getString() {
    advance();
    std::string res;
    while (current && current != '"') {
        res += current;
        advance();
    }
    advance();
    return {res, TT_STRING, lin, col};
}

Token Lexer::getNumber() {
    std::string res;
    TokenKind tk = TT_INTEGER;
    while (current && (std::isdigit(current) || current == '.')) {
        res += current;
        if (current == '.') tk = TT_DOUBLE;
        advance();
    }
    return {res, tk, lin, col};
}

Token Lexer::getIdOrKey() {
    std::string res;
    TokenKind tk = TT_ID;
    while (current && isWord(current)) {
        res += current;
        advance();
    }
    tk = isKey(res)? TT_KEY : TT_ID;
    tk = res == "false" || res == "true"? TT_BOOL : tk;
    tk = res == "null"? TT_NULL : tk;
    return {res, tk, lin, col};
}

Token Lexer::getSymbol(int syntax) {
    std::vector<std::string> unionSymbol = {
        "++", "--",
        "+=", "-=", "/=", "*=", "%=", "&=", "|=", "==", "!="
    };
    if (syntax != SYN_TYPE) {
        unionSymbol.emplace_back(">>");
        unionSymbol.emplace_back("<<");
        unionSymbol.emplace_back(">>=");
        unionSymbol.emplace_back("<<=");
        unionSymbol.emplace_back(">=");
        unionSymbol.emplace_back("<=");
    }
    std::string best;
    for (const auto& i : unionSymbol) {
        if (fcmp(i) && i.size() > best.size())
            best = i;
    }
    if (!best.empty()) {
        advance(best.size());
        return {best, TT_OP, lin, col};
    }
    best += current;
    advance();
    return {best, TT_OP, lin, col};
}
