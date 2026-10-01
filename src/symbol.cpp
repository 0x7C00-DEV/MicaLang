//
// Created by Lenovo on 2026/9/28.
//

#include "../include/symbol.h"

// ---------------- MType ----------------

MType::MType(TP tp) {
    baseType = tp;
}

std::string MType::__str__() {
    return ";";
}

bool MType::operator==(MType* other) {
    return this->__str__() == other->__str__();
}

bool MType::operator!=(MType *other) {
    return !this->operator==(other);
}


FunctionType::FunctionType(MType* retType,
                           std::vector<MType*> argsType)
        : MType(BT_FUNC) {
    this->retType = retType;
    this->argsType = argsType;
}

std::string FunctionType::__str__() {
    // (argTypes;)@retType;
    std::string res = "(";
    for (auto i : argsType)
        res += i->__str__();
    res += ")@" + retType->__str__() + ";";
    return res;
}

TNormalType::TNormalType(Symbol* class_)
        : MType(BT_NORMAL) {
    this->class_ = class_;
}


std::string ClassSymbol::getString() {
    return "$" + name + "/" + clsModule + "/" + std::to_string(clsId);
}

std::string TNormalType::__str__() {
    if (class_->kind == Symbol::SYM_CLASS) {
        auto tmp = ((ClassSymbol *) class_);

        // $NAME/module/id:super; or $NAME/module/id;
        auto res = tmp->getString();
        if (tmp->super) res += ":" + tmp->super->getString();
        return res + ";";
    } else if (class_->kind == Symbol::SYM_INTERFACE) {
        auto tmp = ((InterfaceSymbol *) class_);
        // #NAME/module/id
        return "#" + tmp->name + "/" + tmp->moduleName + "/" + std::to_string(tmp->interfaceId) + ";";
    } else {
        return "ERROR;";
    }
}

TArrayType::TArrayType(MType* elementType, int size)
        : MType(BT_ARRAY) {
    this->elementType = elementType;
    this->size = size;
}

std::string TArrayType::__str__() {
    // [elementType; size];
    return "[" + elementType->__str__() + std::to_string(size) + "];";
}

TTemplateType::TTemplateType(MType* rootType, std::vector<MType*> vars)
        : MType(BT_TEMPLATE) {
    this->rootType = rootType;
    this->vars = vars;
}

std::string TTemplateType::__str__() {
    // <rootType;| templates;>;
    std::string res = "<";
    res += rootType->__str__() + "|";
    for (auto i: vars) {
        res += i->__str__();
    }
    res += ">;";
    return res;
}

Symbol::Symbol(SymbolKind kind) {
    this->kind = kind;
}

FunctionSymbol::FunctionSymbol(std::string name, FunctionType* type)
        : Symbol(SYM_FUNC) {
    this->name = name;
    this->type = type;
}

std::string FunctionSymbol::getString() {
    return std::string();
}

VarSymbol::VarSymbol(std::string name, MType* type, bool isInit, VarKind vkind)
        : Symbol(SYM_VAR) {
    this->name = name;
    this->isInit = isInit;
    this->type = type;
    this->vkind = vkind;
}

ClassSymbol::ClassSymbol(std::string name,
                         ClassSymbol* super,
                         std::vector<InterfaceSymbol*> impl,
                         std::string clsModule,
                         int clsId,
                         std::unordered_map<std::string, MType*> members)
        : Symbol(SYM_CLASS) {
    this->name = name;
    this->super = super;
    this->impl = impl;
    this->members = members;
    this->clsId = clsId;
    this->clsModule = clsModule;
}


ModuleSymbol::ModuleSymbol(std::string modulePath, std::string reName)
        : Symbol(SYM_MODULE) {
    this->modulePath = modulePath;
    this->reName = reName;
}

Scope::Scope(int scopeId, Scope* parent, ScopeKind scopeKind) {
    this->scopeId = scopeId;
    this->parent = parent;
    this->scopeKind = scopeKind;
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

bool Scope::registVar(std::string name, Symbol* sym, int id) {
    if (symbolIsExist(name)) return false;
    if (sym->kind != Symbol::SYM_VAR) return false;
    ((VarSymbol*)sym)->id = id;
    symbols[name] = sym;
    return true;
}

Symbol* Scope::lookupLocalVar(std::string name) {
    if (!symbolIsExist(name))
        return nullptr;
    auto tmp = symbols.find(name);
    if (tmp != symbols.end()) {
        if (tmp->second->kind == Symbol::SYM_VAR)
            return tmp->second;
        return nullptr;
    }
    return parent->lookupLocalVar(name);
}

bool CodeStruct::registSymbolSub(std::string symbol, Symbol *value) {
    if (isExist(symbol)) return false;
    current->symbols[symbol] = value;
    return true;
}

bool CodeStruct::registSymbolGbl(std::string name, Symbol* value) {
    auto tmp = getGlobalScope();
    if (tmp->symbolIsExist(name)) return false;
    tmp->symbols[name] = value;
    return true;
}

bool CodeStruct::isExist(std::string name) {
    return current->symbolIsExist(name);
}

Symbol *CodeStruct::lookup(std::string name) {
    return current->lookupSymbol(name);
}

Scope* CodeStruct::createScope(Scope::ScopeKind kind) {
    ++scopeId;
    current = new Scope(scopeId, current, kind);
    return current;
}

Scope* CodeStruct::leaveScope() {
    if (!current->parent)
        return nullptr;
    current = current->parent;
    return current;
}

CodeStruct::CodeStruct() {
    createScope(Scope::SGLOBAL);
}



Scope *CodeStruct::getGlobalScope() {
    Scope* c = current;
    while (c->parent) c = c->parent;
    return c;
}

InterfaceSymbol::InterfaceSymbol(std::string name, std::unordered_map<std::string, FunctionSymbol *> labels) : Symbol(SYM_INTERFACE){
    this->name = name;
    this->labels = labels;
}

ModuleType::ModuleType(std::string reName, std::string path, Symbol* moduleSymbol) : MType(BT_MODULE){
    this->reName = reName;
    this->path = path;
    this->moduleSymbol = moduleSymbol;
}

std::string ModuleType::__str__() {
    return "@Module?" + path + "?" + reName + "?;";
}

BaseType::BaseType(BaseType::MicaTypes tp) : MType(MType::BT_BASIC){
    this->type = tp;
    switch (tp) {
        case MVOID:
            str = "void";
            break;
        case MINT:
            str = "int";
            break;
        case MDOUBLE:
            str = "double";
            break;
        case MCHAR:
            str = "char";
            break;
        case MBOOL:
            str = "bool";
            break;
    }
}

std::string BaseType::__str__() {
    return str + ";";
}

ClassType::ClassType(std::string name, Symbol *cls) : MType(MType::BT_CLASS){
    this->name = name;
    this->sym = cls;
}

std::string ClassType::__str__() {
    auto tmp = ((ClassSymbol *) sym);

    // $NAME/module/id:super; or $NAME/module/id;
    auto res = tmp->getString();
    if (tmp->super) res += ":" + tmp->super->getString();
    return res + ";";
}
