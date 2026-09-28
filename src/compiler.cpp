//
// Created by Lenovo on 2026/9/25.
//
#include "../include/compiler.h"

ByteCode::ByteCode(int opera) {
    this->opera = opera;
}

ByteCode::ByteCode(std::string label) {
    this->label = label;
    isLabel = true;
}

Instruction::Instruction(std::string index, ByteCode a, ByteCode b, ByteCode c) : a(a), b(b), c(c) {
    this->index = index;
}

int Instruction::toIns() {
    return codingInstr(a.opera, b.opera, c.opera);
}

void CurrentCompileTask::fullBackLabel() {
    std::unordered_map<std::string, int> tmp;
    for (int i = 0; i < ins.size(); ++i)
        tmp[ins[i].index] = i;
    for (int i = 0; i < ins.size(); ++i) {
        if (ins[i].a.isLabel) ins[i].a.opera = tmp[ins[i].a.label];
        if (ins[i].b.isLabel) ins[i].b.opera = tmp[ins[i].b.label];
        if (ins[i].c.isLabel) ins[i].c.opera = tmp[ins[i].c.label];
    }
    for (auto& i : ins)
        i.a.isLabel = i.b.isLabel = i.c.isLabel = false;
}

void CurrentCompileTask::emit(std::string index, ByteCode a, ByteCode b, ByteCode c) {
    ins.push_back(Instruction(index, a, b, c));
}

Function* CurrentCompileTask::getCompileResult() {
    Function *fn = new Function;
    for (auto i : ins)
        fn->ins.push_back(i.toIns());
    fn->name = funcName;
    fn->constants = localConstPool;
    return fn;
}

int CurrentCompileTask::pushLocalConst(MicaValue value) {
    localConstPool.push_back(value);
    return localConstPool.size()-1;
}

CurrentCompileTask::CurrentCompileTask(Scope* current, std::string name) {
    currentScope = functionScope = current;
    this->funcName = name;

}

int CurrentCompileTask::addLocalVar(std::string name, Symbol* sym) {
    if (currentScope->registVar(name, sym, localVarCnt + 1))
        return ++localVarCnt;
    return -1;
}

Symbol* CurrentCompileTask::lookupLocalVar(std::string name) {
    return currentScope->lookupLocalVar(name);
}

std::string CurrentCompileTask::getLabel() {
    return "L" + std::to_string(++localLabelCnt);
}

Scope* Compiler::createScope(Scope::ScopeKind kind) {
    auto tmp = env->cs.createScope(kind);
    getCurrentTsk()->currentScope = tmp;
    return tmp;
}

void Compiler::leaveScope() {
    auto tmp = env->cs.leaveScope();
    getCurrentTsk()->currentScope = tmp? tmp : getCurrentTsk()->functionScope;
}

Compiler::Compiler(CompileEnvironment* environment) {
    this->env = environment;
}

void Compiler::createTask(std::string name) {
    auto tmp = new CurrentCompileTask(env->cs.createScope(Scope::SFUNCTION), name);
    env->tasks.push_back(tmp);
}

CurrentCompileTask *Compiler::getCurrentTsk() {
    return env->tasks.empty() ? nullptr : env->tasks.back();
}