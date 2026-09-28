//
// Created by Lenovo on 2026/9/28.
//

#ifndef MICALANG_SYMBOL_H
#define MICALANG_SYMBOL_H

#include <string>
#include <vector>
#include <unordered_map>
#include "ast.h"

struct Symbol;

struct BaseType {
    enum TP { BT_NORMAL, BT_FUNC, BT_ARRAY, BT_TEMPLATE } baseType;

    BaseType(TP tp);
};

struct FunctionType : BaseType {
    std::vector<BaseType*> varTypes;
    std::vector<BaseType*> argsType;
    BaseType* retType;

    FunctionType(BaseType* retType, std::vector<BaseType*> varType, std::vector<BaseType*> argsType);
};

struct TNormalType : BaseType {
    Symbol* class_;

    TNormalType(Symbol* class_);

};

struct TArrayType : BaseType {
    BaseType* elementType;
    int size;

    TArrayType(BaseType* elementType, int size = -1);

};

struct TTemplateType : BaseType {
    BaseType* rootType;
    std::vector<BaseType*> vars;

    TTemplateType(BaseType* rootType, std::vector<BaseType*> vars);

};

struct Symbol {
    enum SymbolKind { SYM_VAR, SYM_FUNC, SYM_CLASS, SYM_MODULE } kind;

    Symbol(SymbolKind kind);
};

struct FunctionSymbol : Symbol {
    std::string name;
    FunctionType* type;

    FunctionSymbol(std::string name, FunctionType* type);
};

struct VarSymbol : Symbol {
    std::string name;
    BaseType* type;
    bool isInit = false;
    enum VarKind { Global, Local, Arg } vkind;

    VarSymbol(std::string name, BaseType* type, bool isInit, VarKind vkind);
};

struct ClassSymbol : Symbol {
    std::string name;
    ClassSymbol* super;
    ClassSymbol* impl;
    std::unordered_map<std::string, Symbol*> members;

    ClassSymbol(std::string name,
                ClassSymbol* super,
                ClassSymbol* impl,
                std::unordered_map<std::string, Symbol*> members);
};

struct ModuleSymbol : Symbol {
    std::string modulePath;
    std::string reName;

    ModuleSymbol(std::string modulePath, std::string reName);
};

struct Scope {
    Scope* parent = nullptr;
    int scopeId;
    std::unordered_map<std::string, Symbol*> symbols;

    Scope(int scopeId, Scope* parent);
    bool symbolIsExist(std::string);
    Symbol* lookupSymbol(std::string);
};

struct CodeStruct {
    Scope *current= nullptr;
    int scopeId=0;
    CodeStruct();
    bool registSymbol(std::string, Symbol*);
    bool isExist(std::string);
    Symbol* lookup(std::string);
    int createScope();
    bool leaveScope();
};

#endif //MICALANG_SYMBOL_H