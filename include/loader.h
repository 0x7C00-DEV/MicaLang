//
// Created by Lenovo on 2026/9/26.
//

#ifndef MICALANG_LOADER_H
#define MICALANG_LOADER_H
#include "value.h"
#include "symbol.h"

class ProgramLoader {
public:
    ProgramLoader(std::string);
    Program* getData();
    ModuleSymbol* getModuleTag(std::string);
private:
};

#endif //MICALANG_LOADER_H
