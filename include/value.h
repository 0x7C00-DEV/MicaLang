//
// Created by Lenovo on 2026/9/24.
//

#ifndef MICALANG_VALUE_H
#define MICALANG_VALUE_H
#include <cstdint>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "asm.h"

struct Obj;

struct MicaValue {
    union {
        int64_t i;
        double f;
        bool b;
        Obj* obj;
    };
    enum { INT, FLOAT, BOOL, OBJ, NUL } kind;
};

struct Obj {};


struct Function : public Obj {
    int moduleId;
    std::vector<Instr> ins;
    std::vector<MicaValue> constants;
    int argCnt, localCnt;
    std::string name;
};

struct ObjClass : public Obj {
    std::string name;
    ObjClass* super;
    std::unordered_map<std::string, MicaValue> methods;
    std::vector<std::string> fields;
};

struct ObjInstance : public Obj {
    ObjClass* cls;
    std::unordered_map<std::string, MicaValue> fields;
};

#endif //MICALANG_VALUE_H
