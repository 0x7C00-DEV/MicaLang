//
// Created by Lenovo on 2026/9/24.
//

#ifndef MICALANG_VM_HPP
#define MICALANG_VM_HPP
#include "asm.h"
#include "value.h"
#include <algorithm>


class VM;

using CALT = void (VM::*) (int, int);
using BOP = MicaValue (VM::*) (MicaValue, MicaValue);

class VM {
public:
    VM(std::string);
    VM(Module*, std::string, std::vector<MicaValue>);
    void start0(std::string);
    void start0(std::string, std::string, std::vector<MicaValue>);
    void initVec();
    void execute(int);
    void execute(Instr);
    void executeLoop();
    Function* lookFunction(std::string);
    Environment env;
private:
    CALT IVEC[NEW_ARR+1];
    BOP  BINOP[BLESS+1];
    void push(MicaValue);
    MicaValue pop();
    void binOp(int, int);
    void jmp(int, int);
    void jmpf(int, int);
    void jmpt(int, int);
    void call(int, int);
    void loadGVar(int, int);
    void storeGVar(int, int);
    void loadSVar(int, int);
    void storeSVar(int, int);
    void loadGCst(int, int);
    void loadSCst(int, int);
    void imm(int, int);
    void pop_(int, int);
    void bnot(int, int);
    void bitnot(int, int);
    void neg(int, int);
    void ret(int, int);
    void newObj(int, int);
    void dup(int, int);
    void memGet(int, int);
    void memSet(int, int);
    void elGet(int, int);
    void elSet(int, int);
    void newArr(int, int);
    void loadModuleMember(int, int);

    MicaValue loadGlobalConst(int);
    MicaValue loadSubConst(int);
    MicaValue accessSubVar(int);
    MicaValue accessGlobalVar(int);
    void setGlobalVar(int);
    void setSubVar(int);
    void callFunction(Function*, std::vector<MicaValue>);

    MicaValue BADD__(MicaValue, MicaValue);
    MicaValue BSUB__(MicaValue, MicaValue);
    MicaValue BDIV__(MicaValue, MicaValue);
    MicaValue BMUL__(MicaValue, MicaValue);
    MicaValue BSHL__(MicaValue, MicaValue);
    MicaValue BSHR__(MicaValue, MicaValue);
    MicaValue BBAND__(MicaValue, MicaValue);
    MicaValue BBOR__(MicaValue, MicaValue);
    MicaValue BXOR__(MicaValue, MicaValue);
    MicaValue BMOD__(MicaValue, MicaValue);
    MicaValue BEQ__(MicaValue, MicaValue);
    MicaValue BNEQ__(MicaValue, MicaValue);
    MicaValue BAND__(MicaValue, MicaValue);
    MicaValue BOR__(MicaValue, MicaValue);
    MicaValue BEQORBIG__(MicaValue, MicaValue);
    MicaValue BEQORLESS__(MicaValue, MicaValue);
    MicaValue BBIG__(MicaValue, MicaValue);
    MicaValue BLESS__(MicaValue, MicaValue);
};

#endif //MICALANG_VM_HPP
