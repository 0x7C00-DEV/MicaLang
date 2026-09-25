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

struct Function;
struct Obj;

struct MicaValue {
    union {
        int64_t i;
        double f;
        bool b;
        char c;
        Obj* obj;
    };
    enum MTP{ INT, FLOAT, BOOL, OBJ, NUL, CHAR } kind;

    MicaValue(MTP k): i(0) { kind = k; }

    MicaValue(): i(0), kind(NUL) {}

    static MicaValue Int(int64_t x)  { MicaValue r{INT}; r.i = x;  r.kind = INT;   return r; }
    static MicaValue Float(double x) { MicaValue r{FLOAT}; r.f = x;  r.kind = FLOAT; return r; }
    static MicaValue Bool(bool x)    { MicaValue r{BOOL}; r.b = x;  r.kind = BOOL;  return r; }
    static MicaValue Null()          { MicaValue r{NUL}; r.i = 0;  r.kind = NUL;   return r; }
    static MicaValue Char(char x)    { MicaValue r{CHAR}; r.c = x;  r.kind = CHAR;   return r; }
    static MicaValue Object(Obj* x)  { MicaValue r{OBJ}; r.obj = x; r.kind = OBJ;  return r; }
};

struct Obj {
    enum ObjTP { NORMAL, FUNCTION } tp;
    Obj(ObjTP tp) {
        this->tp = tp;
    }
};

struct ObjArray : Obj {
    std::vector<MicaValue> elements;
    ObjArray(): Obj(NORMAL) {}
};

struct ObjString : ObjArray {
    ObjString(std::string str) {
        for (auto i : str)
            elements.push_back(MicaValue::Char(i));
    }
};

struct Program {
    std::vector<Function*> funcs;
    std::vector<MicaValue> constPools;
};

struct Module {
    std::string moduleName;
    std::vector<Function*> funcs;
    std::vector<MicaValue> globalConstPool;
    std::vector<MicaValue> globalVars;
};

struct Frame {
    int pc=0;
    Frame* caller;
    std::vector<MicaValue> mstack;
    Function* fn = nullptr;
    std::vector<MicaValue> localVar;
    Module* module;
    Frame(Function*, Frame*);
    Instr getInstr();
};

struct Environment {
    Module* mainModule;
    std::vector<Module*> modules;
    std::vector<Frame*> callChain;
    Frame* getCTask();
    void loadModule(Program*, std::string);
};

struct Function : Obj {
    Module* module=nullptr;
    std::vector<Instr> ins;
    std::vector<MicaValue> constants;
    std::string name;
    Function(): Obj(FUNCTION) {}
};

struct ObjClass : Obj {
    std::string name;
    ObjClass* super;
    std::unordered_map<std::string, MicaValue> methods;
    std::vector<std::string> fields;
};

struct ObjInstance : Obj {
    ObjClass* cls;
    std::unordered_map<std::string, MicaValue> fields;
};

#endif //MICALANG_VALUE_H
