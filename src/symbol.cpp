//
// Created by Lenovo on 2026/9/28.
//

#include "../include/symbol.h"

// ---------------- MType ----------------


std::string getTypeString(MType* a) {
    return a->__str__();
}

InterfaceType::InterfaceType(InterfaceSymbol* s): MType(BT_INTERFACE) {
    this->name = s->name;
    this->symbol = s;
}

MType::MType(TP tp) {
    baseType = tp;
}

std::string MType::__str__() {
    return ";";
}

TImplementsType::TImplementsType(std::vector<InterfaceSymbol*> interface)
        : MType(BT_IMPL) {
    this->interfaces = interface;
}

std::string inttostr(InterfaceSymbol* a) {
    return "[" + a->name + "@" + a->moduleName + "@" + std::to_string(a->interfaceId) + "]";
}

std::string InterfaceType::__str__() {
    return inttostr(this->symbol);
}

std::string TImplementsType::__str__() {
    std::string res = "%";
    for (auto i : interfaces)
        res += inttostr(i);
    return res + ";";
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
        res += getTypeString(i);
    res += ")@" + getTypeString(retType) + ";";
    return res;
}

std::string ClassSymbol::getString() {
    return "$" + name + "/" + clsModule + "/" + std::to_string(clsId);
}

TArrayType::TArrayType(MType* elementType, int size)
        : MType(BT_ARRAY) {
    this->elementType = elementType;
    this->size = size;
}

std::string TArrayType::__str__() {
    // [elementType; size];
    return "[" + getTypeString(elementType) + "];";
}

TTemplateType::TTemplateType(MType* rootType, std::vector<MType*> vars)
        : MType(BT_TEMPLATE) {
    this->rootType = rootType;
    this->vars = vars;
}

std::string TTemplateType::__str__() {
    // <rootType;| templates;>;
    std::string res = "<";
    res += getTypeString(rootType) + "|";
    for (auto i: vars) {
        res += getTypeString(i);
    }
    res += ">;";
    return res;
}

Symbol::Symbol(SymbolKind kind) {
    this->kind = kind;
}

FunctionSymbol::FunctionSymbol(std::string name, FunctionType* type, bool isClassMethod)
        : Symbol(SYM_FUNC) {
    this->name = name;
    this->type = type;
    this->isClassMethod = isClassMethod;
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


void ClassSymbol::checkIsImplement() {
    for (auto i : impl)
        for (auto j : i->labels)
            if (!checkFuncIsExist(j.second)) {
                std::cout << "ERROR: The function '" << j.first << "' hasn't been overridden " << std::endl;
                exit(-1);
            }
}


FunctionType* FunctionType::withOutSelf() {
    std::vector<MType*> at;
    if (argsType.size() > 1) {
        for (int i=1; i<argsType.size(); ++i)
            at.push_back(argsType[i]);
    }
    return new FunctionType(retType, at);
}

bool ClassType::isImplement(std::vector<InterfaceSymbol*> tsym) {
    for (auto i : tsym)
        for (auto j : i->labels)
            if (!((ClassSymbol*)sym)->checkFuncIsExist(j.second))
                return false;
    return true;
}

bool ClassSymbol::checkFuncIsExist(FunctionSymbol* func) {
    auto it = members.find(func->name);
    if (it != members.end() && it->second->baseType == MType::BT_FUNC
        && ((FunctionType*)it->second)->withOutSelf()->__str__()
           == ((FunctionType*)func->type)->withOutSelf()->__str__())
        return true;
    if (super) return super->checkFuncIsExist(func);
    return false;
}

ClassSymbol::ClassSymbol(std::string name,
                         ClassSymbol* super,
                         std::vector<InterfaceSymbol*> impl,
                         std::string clsModule,
                         int clsId,
                         std::unordered_map<std::string, MType*> members,
                         std::unordered_map<std::string, Symbol*> symbols)
        : Symbol(SYM_CLASS) {
    this->name = name;
    this->super = super;
    this->impl = impl;
    this->members = members;
    this->clsId = clsId;
    this->clsModule = clsModule;
    this->syms = symbols;
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
        case MNULL:
            str = "null";
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

bool typeComp(MType* a, MType* b) {
    if (a->baseType == MType::BT_CLASS && b->baseType == MType::BT_IMPL)
        return ((ClassType*)a)->isImplement(((TImplementsType*)b)->interfaces);
    return a->__str__() == b->__str__() || (a->baseType == MType::BT_CLASS
        && b->baseType == MType::BT_INTERFACE
        && ((ClassType*)a)->isImplement({((InterfaceType*)b)->symbol}));
}

bool typeComp(MType* a, std::vector<InterfaceSymbol*> b) {
    if (a->baseType != MType::BT_CLASS) {
        std::cout << "ERROR: not a class\n";
        exit(-1);
    }
    return ((ClassType*)a)->isImplement(b);
}
