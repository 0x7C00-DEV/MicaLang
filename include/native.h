//
// Created by Lenovo on 2026/10/2.
//

#ifndef NATIVE_H
#define NATIVE_H
#include "config.h"
#include "symbol.h"
#include "value.h"
#include "random"

int random_int(int , int );

#ifdef TEST

class NativeFunction {
public:
    std::string name;
    Function* nativeFn;
    FunctionSymbol* symbol;
    NativeFunction(std::string, Function*, FunctionSymbol*);
};

std::vector<NativeFunction*> getFuncs();


#endif

#endif //NATIVE_H
