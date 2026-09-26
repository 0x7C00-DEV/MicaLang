//
// Created by Lenovo on 2026/9/26.
//

#ifndef MICALANG_LOADER_H
#define MICALANG_LOADER_H
#include "value.h"

class ProgramLoader {
public:
    ProgramLoader(std::string);
    Program* getData();
private:
};

#endif //MICALANG_LOADER_H
