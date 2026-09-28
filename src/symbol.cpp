//
// Created by Lenovo on 2026/9/28.
//

#include "../include/symbol.h"

// ---------------- BaseType ----------------

BaseType::BaseType(TP tp) {
    baseType = tp;
}

FunctionType::FunctionType(BaseType* retType,
                           std::vector<BaseType*> varType,
                           std::vector<BaseType*> argsType)
        : BaseType(BT_FUNC) {
    this->retType = retType;
    this->varTypes = varType;
    this->argsType = argsType;
}

TNormalType::TNormalType(Symbol* class_)
        : BaseType(BT_NORMAL) {
    this->class_ = class_;
}

TArrayType::TArrayType(BaseType* elementType, int size)
        : BaseType(BT_ARRAY) {
    this->elementType = elementType;
    this->size = size;
}

TTemplateType::TTemplateType(BaseType* rootType, std::vector<BaseType*> vars)
        : BaseType(BT_TEMPLATE) {
    this->rootType = rootType;
    this->vars = vars;
}

Symbol::Symbol(SymbolKind kind) {
    this->kind = kind;
}

FunctionSymbol::FunctionSymbol(std::string name, FunctionType* type)
        : Symbol(SYM_FUNC) {
    this->name = name;
    this->type = type;
}

VarSymbol::VarSymbol(std::string name, BaseType* type, bool isInit, VarKind vkind)
        : Symbol(SYM_VAR) {
    this->name = name;
    this->isInit = isInit;
    this->type = type;
    this->vkind = vkind;
}

ClassSymbol::ClassSymbol(std::string name,
                         ClassSymbol* super,
                         ClassSymbol* impl,
                         std::unordered_map<std::string, Symbol*> members)
        : Symbol(SYM_CLASS) {
    this->name = name;
    this->super = super;
    this->impl = impl;
    this->members = members;
}

ModuleSymbol::ModuleSymbol(std::string modulePath, std::string reName)
        : Symbol(SYM_MODULE) {
    this->modulePath = modulePath;
    this->reName = reName;
}

Scope::Scope(int scopeId, Scope* parent) {
    this->scopeId = scopeId;
    this->parent = parent;
}

Symbol *Scope::lookupSymbol(std::string name) {
    if (!symbolIsExist(name))
        return nullptr;
    auto tmp = symbols.find(name);
    return tmp == symbols.end()? parent->lookupSymbol(name) : tmp->second;
}

bool Scope::symbolIsExist(std::string name) {
    if (symbols.find(name) != symbols.end())
        return true;
    if (parent) return parent->symbolIsExist(name);
    return false;
}

bool CodeStruct::registSymbol(std::string symbol, Symbol *value) {
    if (isExist(symbol)) return false;
    current->symbols[symbol] = value;
    return true;
}

bool CodeStruct::isExist(std::string name) {
    return current->symbolIsExist(name);
}

Symbol *CodeStruct::lookup(std::string name) {
    return current->lookupSymbol(name);
}

int CodeStruct::createScope() {
    ++scopeId;
    current = new Scope(scopeId, current);
    return scopeId;
}

bool CodeStruct::leaveScope() {
    if (!current->parent)
        return false;
    current = current->parent;
    return true;
}

CodeStruct::CodeStruct() {
    createScope();
}
