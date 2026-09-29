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

struct MType {
    enum TP { BT_BASIC, BT_NORMAL, BT_FUNC, BT_ARRAY, BT_TEMPLATE, BT_MODULE, BT_CLASS } baseType;

    MType(TP tp);
    virtual std::string __str__() = 0;

    bool operator==(MType*);
    bool operator!=(MType*);
};

struct BaseType : MType {
    enum MicaTypes { MINT, MDOUBLE, MCHAR, MBOOL, MVOID } type;
    std::string str;
    std::string __str__() override;
    BaseType(MicaTypes);
};

struct ModuleType : MType {
    std::string path;
    std::string reName;
    ModuleType(std::string, std::string);

    std::string __str__() override;
};

struct FunctionType : MType {
    std::vector<MType*> templateTypes;
    std::vector<MType*> argsType;
    MType* retType;
    std::string __str__() override;
    FunctionType(MType* retType, std::vector<MType*> templates, std::vector<MType*> argsType);
};

struct TNormalType : MType {
    Symbol* class_;

    TNormalType(Symbol* class_);
    std::string __str__() override;
};

struct TArrayType : MType {
    MType* elementType;
    int size;

    TArrayType(MType* elementType, int size = -1);
    std::string __str__() override;
};

struct TTemplateType : MType {
    MType* rootType;
    std::vector<MType*> vars;

    TTemplateType(MType* rootType, std::vector<MType*> vars);
    std::string __str__() override;
};

struct Symbol {
    enum SymbolKind { SYM_VAR, SYM_FUNC, SYM_CLASS, SYM_MODULE, SYM_INTERFACE } kind;

    Symbol(SymbolKind kind);
};

struct FunctionSymbol : Symbol {
    std::string name;
    FunctionType* type;

    FunctionSymbol(std::string name, FunctionType* type);
};

struct VarSymbol : Symbol {
    std::string name;
    MType* type;
    int id=0;
    bool isInit = false;
    enum VarKind { Global, Local, Arg } vkind;

    VarSymbol(std::string name, MType* type, bool isInit, VarKind vkind);
};

struct InterfaceSymbol : Symbol {
    std::string name;
    std::string moduleName;
    int interfaceId;
    std::unordered_map<std::string, FunctionSymbol*> labels;
    InterfaceSymbol(std::string, std::unordered_map<std::string, FunctionSymbol*>);
};

struct ClassSymbol : Symbol {
    std::string name;
    ClassSymbol* super;
    std::vector<InterfaceSymbol*> impl;
    std::unordered_map<std::string, Symbol*> members;
    std::string clsModule;
    int clsId=0;
    std::string getString();
    ClassSymbol(std::string name,
                ClassSymbol* super,
                std::vector<InterfaceSymbol*> impl,
                std::string clsModule,
                int clsId,
                std::unordered_map<std::string, Symbol*> members);
};

struct ClassType : MType {
    Symbol* sym;
    std::string name;
    std::string __str__() override;
    ClassType(std::string, Symbol*);
};

struct ModuleSymbol : Symbol {
    std::string modulePath;
    std::string reName;
    std::unordered_map<std::string, ClassSymbol*> cls;
    std::unordered_map<std::string, FunctionSymbol*> funcs;
    ModuleSymbol(std::string , std::string );
};

struct Scope {
    enum ScopeKind { SNORMAL_BLOCK, SGLOBAL, SFUNCTION, SCLASS } scopeKind;
    Scope* parent = nullptr;
    int scopeId;
    std::unordered_map<std::string, Symbol*> symbols;
    bool registVar(std::string, Symbol*, int);
    Symbol* lookupLocalVar(std::string);
    Scope(int scopeId, Scope* parent, ScopeKind scopeKind);
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
    Scope* createScope(Scope::ScopeKind);
    Scope* leaveScope();
};

#endif //MICALANG_SYMBOL_H