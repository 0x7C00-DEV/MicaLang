//
// Created by Lenovo on 2026/9/24.
//

#ifndef MICALANG_POSITION_HPP
#define MICALANG_POSITION_HPP
#include <iostream>

class Position {
public:
    std::string file;
    int lin, col;
    Position() : file("UNKNOWN"), lin(-1), col(-1) {}
    Position(std::string, int, int);
};

#endif //MICALANG_POSITION_HPP