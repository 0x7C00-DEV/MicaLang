//
// Created by Lenovo on 2026/9/11.
//
#include <utility>

#include "../include/token.h"

Token::Token(std::string data, const TokenKind kind, int lin, int col) {
    this->kind = kind;
    this->data = std::move(data);
    this->lin = lin;
    this->col = col;
}

Token::Token() {

}

void Token::debug() const {
    std::cout << "Token( '" << data << "', " << kind << " )" << std::endl;
}

const std::vector<std::string> micaKey = {
    "if", "else", "for", "while", "switch", "case", "do",
    "fn", "class", "enum", "interface",
    "public", "private", "protected", "extend", "implement",
    "return", "goto", "native", "import", "in", "as", "let"
};

bool isKey(const std::string& name) {
    for (const auto& i : micaKey)
        if (i == name)
            return true;
    return false;
}
