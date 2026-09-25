//
// Created by Lenovo on 2026/9/11.
//
#include <utility>

#include "../include/token.h"

Token::Token(std::string data, const TokenKind kind, Position  begin, Position  end): begin(std::move(begin)), end(std::move(end)) {
    this->kind = kind;
    this->data = std::move(data);
}

Token::Token(): begin({"UNKNOWN", 0,0}), end({"UNKNOWN", 0,0}) {

}

void Token::debug() const {
    std::cout << "Token( '" << data << "', " << kind << " )" << std::endl;
}

const std::vector<std::string> micaKey = {
    "if", "else", "for", "while", "switch", "case", "do",
    "fn", "class", "enum", "interface",
    "public", "private", "protected", "extend", "implement",
    "return", "goto", "native", "import", "in", "as", "let",
    "continue", "break", "default", "new"
};

bool isKey(const std::string& name) {
    for (const auto& i : micaKey)
        if (i == name)
            return true;
    return false;
}
