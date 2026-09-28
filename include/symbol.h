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
    enum TP { BT_NORMAL, BT_FUNC, BT_ARRAY, BT_TEMPLATE, BT_MODULE } baseType;

    BaseType(TP tp);
    virtual std::string __str__() = 0;

    bool operator==(BaseType*);
    bool operator!=(BaseType*);
};

struct ModuleType : BaseType {
    std::string path;
    std::string reName;
    ModuleType(std::string, std::string);

    std::string __str__() override;
};

struct FunctionType : BaseType {
    std::vector<BaseType*> templateTypes;
    std::vector<BaseType*> argsType;
    BaseType* retType;
    std::string __str__() override;
    FunctionType(BaseType* retType, std::vector<BaseType*> templates, std::vector<BaseType*> argsType);
};

struct TNormalType : BaseType {
    Symbol* class_;

    TNormalType(Symbol* class_);
    std::string __str__() override;
};

struct TArrayType : BaseType {
    BaseType* elementType;
    int size;

    TArrayType(BaseType* elementType, int size = -1);
    std::string __str__() override;
};

struct TTemplateType : BaseType {
    BaseType* rootType;
    std::vector<BaseType*> vars;

    TTemplateType(BaseType* rootType, std::vector<BaseType*> vars);
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
    BaseType* type;
    int id=0;
    bool isInit = false;
    enum VarKind { Global, Local, Arg } vkind;

    VarSymbol(std::string name, BaseType* type, bool isInit, VarKind vkind);
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

struct ModuleSymbol : Symbol {
    std::string modulePath;
    std::string reName;
    std::unordered_map<std::string, ClassSymbol*> cls;
    std::unordered_map<std::string, FunctionSymbol*> funcs;
    // 规定：模块之间只允许访问类与函数，其他的一律为非法访问。
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