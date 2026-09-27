//
// Created by Lenovo on 2026/9/27.
//
#include "../include/symbol.h"


Symbol::Symbol(SymbolKind kind) {
    this->kind = kind;
}

VarSymbol::VarSymbol(int address, bool isGlobal, std::string name, Scope* scope): Symbol(SVAR) {
    this->isGlobal = isGlobal;
    this->name = name;
    this->constPoolAddress = address;
    this->scope = scope;
}

FunctionSymbol::FunctionSymbol(std::string name, int address, Scope* scope): Symbol(SFUNCTION) {
    this->name = name;
    this->scope = scope;
    this->constPoolAddress = address;
}


ObjectSymbol::ObjectSymbol(std::string name, int address, Scope* scope): Symbol(SOBJECT) {
    this->name = name;
    this->scope = scope;
    this->constPoolAddress = address;
}


EnumSymbol::EnumSymbol(std::string name, int address, Scope* scope): Symbol(SENUM) {
    this->name = name;
    this->scope = scope;
    this->constPoolAddress = address;
}

InterfaceSymbol::InterfaceSymbol(std::string name, int address, Scope* scope): Symbol(SINTERFACE) {
    this->name = name;
    this->scope = scope;
    this->constPoolAddress = address;
}


ModuleSymbol::ModuleSymbol(std::string align, std::string path, int address, Scope* scope): Symbol(SMODULE) {
    this->align = align;
    this->path = path;
    this->constPoolAddress = address;
    this->scope = scope;
}


Scope::Scope(int* a) {
    scopeId = *a;
    ++*a;
}

void Scope::addSymbol(std::string name, Symbol* value) {
    symbols[name] = value;
}

bool Scope::isExist(std::string name, Symbol::SymbolKind kind) {
    auto tmp = symbols.find(name);
    if (tmp != symbols.end() && tmp->second->kind == kind)
        return true;
    for (auto i : childScope)
        if (i->isExist(name, kind))
            return true;
    return false;
}

bool Scope::isExist(std::string name) {
    auto tmp = symbols.find(name);
    if (tmp != symbols.end())
        return true;
    for (auto i : childScope)
        if (i->isExist(name))
            return true;
    return false;
}

Symbol* Scope::getSymbol(std::string name) {
    if (!isExist(name)) {
        E:
        std::cout << "Name '" << name << "' not exist in scope '" << scopeId << "'\n";
        exit(-1);
    }
    auto tmp = symbols.find(name);
    if (tmp != symbols.end())
        return tmp->second;
    for (auto i : childScope)
        if (i->isExist(name))
            return i->getSymbol(name);
    goto E;
}


Symbol* Scope::getSymbol(std::string name, Symbol::SymbolKind kind) {
    if (!isExist(name, kind)) {
        E:
        std::cout << "Name '" << name << "' not exist in scope '" << scopeId << "'\n";
        exit(-1);
    }
    auto tmp = symbols.find(name);
    if (tmp != symbols.end())
        if (tmp->second->kind == kind)
            return tmp->second;
    for (auto i : childScope)
        if (i->isExist(name, kind))
            return i->getSymbol(name, kind);
    goto E;
}