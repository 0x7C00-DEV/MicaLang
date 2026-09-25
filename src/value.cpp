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


std::vector<MicaCFunction*> loadMicaFunctions(const char* dllPath) {
    HMODULE h = LoadLibraryA(dllPath);
    BYTE* base = (BYTE*)h;

    auto dos = (IMAGE_DOS_HEADER*)base;
    auto nt  = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    auto dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    auto exp = (IMAGE_EXPORT_DIRECTORY*)(base + dir.VirtualAddress);

    auto funcs = (DWORD*)(base + exp->AddressOfFunctions);
    auto names = (DWORD*)(base + exp->AddressOfNames);
    auto ords  = (WORD*) (base + exp->AddressOfNameOrdinals);

    std::vector<MicaCFunction*> result;
    for (DWORD i = 0; i < exp->NumberOfNames; i++)
        result.push_back(
            reinterpret_cast<MicaCFunction*>(base + funcs[ords[i]])
        );
    return result;
}

Module::Module() {

}

Module::Module(std::string path) {
    std::vector<MicaCFunction*> tmp = loadMicaFunctions(path.c_str());
    for (auto i : tmp) funcs.push_back(new Function(i));
}

void Frame::__call__(Environment* env, std::vector<MicaValue> args) {
    caller->mstack.push_back(fn->__native__(env, args));
}
