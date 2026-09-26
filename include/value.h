//
// Created by Lenovo on 2026/9/24.
//

#ifndef MICALANG_VALUE_H
#define MICALANG_VALUE_H
#include <cstdint>
#include <vector>
#include <windows.h>
#include <imagehlp.h>
#include <iostream>
#include <unordered_map>
#include "asm.h"
#include "config.h"

struct Frame;
struct Module;
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
    Obj* next = nullptr;
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

struct Environment {
    Module* mainModule;
    std::vector<Module*> modules;
    std::vector<Frame*> callChain;
    Obj* heapHead = nullptr;
    Obj* heapEnd = nullptr;
    Obj* malloc(Obj::ObjTP);
    Frame* getCTask();
    Module* loadModule(Program*, std::string);
};

using MicaCFunction = MicaValue(Environment*, std::vector<MicaValue>);

std::vector<MicaCFunction*> loadMicaFunctions(const char*);

struct Frame {
    int pc=0;
    Frame* caller;
    std::vector<MicaValue> mstack;
    Function* fn = nullptr;
    std::vector<MicaValue> localVar;
    Module* module;
    Frame(Function*, Frame*);
    Instr getInstr();

    void __call__(Environment*, std::vector<MicaValue>);
};

struct Function : Obj {
    Module* module=nullptr;
    std::vector<Instr> ins;
    std::vector<MicaValue> constants;
    std::string name;
    MicaCFunction* __native__;
    bool isNative;

    Function(): Obj(FUNCTION) {
        isNative = false;
    }

    Function(MicaCFunction* n): Obj(FUNCTION) {
        __native__ = n;
        isNative = true;
    }
};

struct Module {
    std::string moduleName;
    std::vector<MicaValue> globalConstPool;
    std::vector<MicaValue> globalVars;
    std::vector<Function*> funcs;

    Module();

#ifdef SUPDLL
    Module(std::string);
#endif
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
    MicaValue getField(std::string);
    void setField(std::string, MicaValue);
};

#endif //MICALANG_VALUE_H
