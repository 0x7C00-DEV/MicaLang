//
// Created by Lenovo on 2026/9/24.
//
#include "../include/value.h"
#include "../include/vm.h"


MicaValue* ObjClass::findMethod(std::string _name) {
    if (methods.find(_name) != methods.end())
        return &methods[_name];
    if (super) return super->findMethod(_name);
    return nullptr;
}

MicaValue ObjInstance::getField(std::string name) {
    if (fields.find(name) != fields.end())
        return fields[name];
    auto tmp = cls->findMethod(name);
    if (!tmp) {
        std::cout << "Error: class '" << cls->name << "' has not field '" << name << "'\n";
        exit(-1);
    }
    return* tmp;
}

void ObjInstance::setField(std::string name, MicaValue value) {
    fields[name] = value;
}

Frame::Frame(Function* fn, Frame* caller) {
    this->fn = fn;
    this->module = fn->module;
    this->caller = caller;
}

Frame* Environment::getCTask() {
    return callChain.back();
}

void Environment::registModule(std::string path, std::string align, Module* module) {
    ptoa[path] = align;
    atop[align] = path;
    isImport[align] = module;
}

Module* Environment::loadModule(VM* vm, Program* md, std::string name, std::string path) {
    if (isImport.find(name) != isImport.end()) return isImport[name];
    Module* module = new Module();
    module->moduleName = name;
    module->globalConstPool = md->constPools;
    for (auto i : md->funcs) {
        i->module = module;
        module->funcs.push_back(i);
    }
    registModule(path, name, module);
    modules.push_back(module);
    if (!module->isReady)
        vm->initModule(module);
    return module;
}

void Environment::addObject(Obj* obj) {
    if (!heapHead) {
        heapHead = heapEnd = obj;
        return;
    }
    heapEnd->next = obj;
    heapEnd = heapEnd->next;
}

Instr Frame::getInstr() {
    return fn->ins[pc++];
}

ObjInstance::ObjInstance(): Obj(INITED_OBJECT) {

}

#ifdef SUPDLL
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
#endif

Module::Module() : Obj(MODULE) {

}

ObjClass::ObjClass(std::string name): Obj(USER_DEFING_CLASS) {
    this->name = name;
}

#ifdef SUPDLL
Module::Module(std::string path): Obj(MODULE) {
    std::vector<MicaCFunction*> tmp = loadMicaFunctions(path.c_str());
    for (auto i : tmp) funcs.push_back(new Function(i));
}
#endif

void Frame::__call__(Environment* env, std::vector<MicaValue> args) {
    auto tmp = fn->__native__(env, args);
    if (caller) caller->mstack.push_back(tmp);
}
