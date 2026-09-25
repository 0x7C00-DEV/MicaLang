//
// Created by Lenovo on 2026/9/24.
//
#include "../include/value.h"
#include "../include/vm.h"

Frame::Frame(Function* fn, Frame* caller) {
    this->fn = fn;
    this->module = fn->module;
    this->caller = caller;
}

Frame* Environment::getCTask() {
    return callChain.back();
}

void Environment::loadModule(Program* md, std::string name) {
    Module* module = new Module();
    module->moduleName = name;
    module->globalConstPool = md->constPools;
    for (auto i : md->funcs) {
        i->module = module;
        module->funcs.push_back(i);
    }
    VM vm(module, "@init", {});
    modules.push_back(module);
}

Instr Frame::getInstr() {
    return fn->ins[pc++];
}