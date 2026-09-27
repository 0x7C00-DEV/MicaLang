//
// Created by Lenovo on 2026/9/27.
//

#ifndef MICALANG_SYMBOL_H
#define MICALANG_SYMBOL_H
#include <vector>
#include "value.h"
#include <iostream>

struct Scope;

struct Symbol {
    enum SymbolKind { SFUNCTION, SVAR, SOBJECT, SENUM, SINTERFACE, SMODULE } kind;
    Symbol(SymbolKind);
};

struct VarSymbol : Symbol {
    bool isGlobal = false;
    int constPoolAddress = 0;
    std::string name;
    Scope* scope;
    VarSymbol(int, bool, std::string, Scope*);
};

struct FunctionSymbol : Symbol {
    int constPoolAddress = 0;
    std::string name;
    Scope* scope;
    FunctionSymbol(std::string, int, Scope*);
};

struct ObjectSymbol : Symbol {
    std::string name;
    int constPoolAddress = 0;
    Scope* scope;
    ObjectSymbol(std::string, int, Scope*);
};

struct EnumSymbol : Symbol {
    std::string name;
    int constPoolAddress = 0;
    Scope* scope;
    EnumSymbol(std::string, int, Scope*);
};

struct InterfaceSymbol : Symbol {
    std::string name;
    int constPoolAddress = 0;
    Scope* scope;
    InterfaceSymbol(std::string, int, Scope*);
};

struct ModuleSymbol : Symbol {
    std::string align;
    std::string path;
    int constPoolAddress = 0;
    Scope* scope;
    ModuleSymbol(std::string, std::string, int, Scope*);
};

struct Scope {
    int scopeId;
    Scope* superScope;
    std::vector<Scope*> childScope;
    std::unordered_map<std::string, Symbol*> symbols;
    void addSymbol(std::string, Symbol*);
    bool isExist(std::string, Symbol::SymbolKind);
    bool isExist(std::string);
    Symbol* getSymbol(std::string);
    Symbol* getSymbol(std::string, Symbol::SymbolKind);
    Scope(int*);
};

#endif //MICALANG_SYMBOL_H
