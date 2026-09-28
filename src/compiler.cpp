//
// Created by Lenovo on 2026/9/25.
//
#include "../include/compiler.h"

ByteCode::ByteCode(int opera) {
    this->opera = opera;
}

ByteCode::ByteCode(std::string label) {
    this->label = label;
    isLabel = true;
}

Instruction::Instruction(std::string index, ByteCode a, ByteCode b, ByteCode c) : a(a), b(b), c(c) {
    this->index = index;
}

int Instruction::toIns() {
    return codingInstr(a.opera, b.opera, c.opera);
}

void CurrentCompileTask::fullBackLabel() {
    std::unordered_map<std::string, int> tmp;
    for (int i = 0; i < ins.size(); ++i)
        tmp[ins[i].index] = i;
    for (int i = 0; i < ins.size(); ++i) {
        if (ins[i].a.isLabel) ins[i].a.opera = tmp[ins[i].a.label];
        if (ins[i].b.isLabel) ins[i].b.opera = tmp[ins[i].b.label];
        if (ins[i].c.isLabel) ins[i].c.opera = tmp[ins[i].c.label];
    }
    for (auto& i : ins)
        i.a.isLabel = i.b.isLabel = i.c.isLabel = false;
}

void CurrentCompileTask::emit(std::string index, ByteCode a, ByteCode b, ByteCode c) {
    ins.push_back(Instruction(index, a, b, c));
}

Function* CurrentCompileTask::getCompileResult() {
    Function *fn = new Function;
    for (auto i : ins)
        fn->ins.push_back(i.toIns());
    fn->name = funcName;
    fn->constants = localConstPool;
    return fn;
}

int CurrentCompileTask::pushLocalConst(MicaValue value) {
    localConstPool.push_back(value);
    return localConstPool.size()-1;
}

CurrentCompileTask::CurrentCompileTask(Scope* current, std::string name) {
    currentScope = functionScope = current;
    this->funcName = name;

}

int CurrentCompileTask::addLocalVar(std::string name, Symbol* sym) {
    if (currentScope->registVar(name, sym, localVarCnt + 1))
        return ++localVarCnt;
    return -1;
}

Symbol* CurrentCompileTask::lookupLocalVar(std::string name) {
    return currentScope->lookupLocalVar(name);
}

std::string CurrentCompileTask::getLabel() {
    return "L" + std::to_string(++localLabelCnt);
}

Scope* Compiler::createScope(Scope::ScopeKind kind) {
    auto tmp = env->cs.createScope(kind);
    getCurrentTsk()->currentScope = tmp;
    return tmp;
}

void Compiler::leaveScope() {
    auto tmp = env->cs.leaveScope();
    getCurrentTsk()->currentScope = tmp? tmp : getCurrentTsk()->functionScope;
}

Compiler::Compiler(CompileEnvironment* environment) {
    this->env = environment;
    opera["+"] = BADD;
    opera["-"] = BSUB;
    opera["/"] = BDIV;
    opera["*"] = BMUL;
    opera["<<"] = BSHL;
    opera[">>"] = BSHR;
    opera["%"] = BMOD;
    opera["^"] = BXOR;
    opera["=="] = BEQ;
    opera["!="] = BNEQ;
    opera[">"] = BBIG;
    opera["<"] = BLESS;
    opera[">="] = BEQORBIG;
    opera["<="] = BEQORLESS;
    opera["&"] = BBAND;
    opera["|"] = BBOR;
    opera["&&"] = BAND;
    opera["||"] = BOR;
}

void Compiler::createTask(std::string name) {
    auto tmp = new CurrentCompileTask(env->cs.createScope(Scope::SFUNCTION), name);
    env->tasks.push_back(tmp);
}

CurrentCompileTask *Compiler::getCurrentTsk() {
    return env->tasks.empty() ? nullptr : env->tasks.back();
}

MType* Compiler::visitBinOpNode(AST* a) {

} 

MType* Compiler::visitCallNode(AST* a) {

}

MType* Compiler::visitElementGet(AST* a) {

}

MType* Compiler::visitMemberAccess(AST* a) {

}

MType* Compiler::visitFunction(AST* a) {

}

MType* Compiler::visitValue(AST* a) {
    if (a->kind == AST::AST_ARRAY)
        return visitArray(a);
    if (a->kind == AST::AST_BIN_OP)
        return visitBinOpNode(a);
    if (a->kind == AST::AST_MEMBER_ACCESS)
        return visitMemberAccess(a);
    if (a->kind == AST::AST_ELEMENT_GET)
        return visitElementGet(a);
    if (a->kind == AST::AST_ASSIGN_NODE)
        return visitAssign(a);
    if (a->kind == AST::AST_BOOL)
        return visitBool(a);
    if (a->kind == AST::AST_CALL)
        return visitCallNode(a);
    if (a->kind == AST::AST_DIGIT)
        return visitNumber(a);
    if (a->kind == AST::AST_THREE_OP)
        return visitTernOp(a);
    if (a->kind == AST::AST_CHAR)
        return visitChar(a);
    if (a->kind == AST::AST_NEW_CLASS)
        return visitNewClass(a);
    std::cout << "not suppose\n";
    return nullptr;
}

MType* Compiler::visitAssign(AST* a) {

}

MType* Compiler::visitStmt(AST* a) {
    if (a->kind == AST::AST_FUNC_DEF)
        return visitFunction(a);
    if (a->kind == AST::AST_FOR)
        return visitForLoop(a);
    if (a->kind == AST::AST_DO_WHILE)
        return visitDoWhile(a);
    if (a->kind == AST::AST_WHILE)
        return visitWhileLoop(a);
    if (a->kind == AST::AST_SWITCH)
        return visitSwitch(a);
    if (a->kind == AST::AST_RETURN)
        return visitReturn(a);
    if (a->kind == AST::AST_CLASS)
        return visitClass(a);
    if (a->kind == AST::AST_INTERFACE)
        return visitClass(a);
    if (a->kind == AST::AST_INTERFACE)
        return visitInterface(a);
    if (a->kind == AST::AST_FUNC_TAG)
        return visitFuncTag(a);
    return visitValue(a);
}

MType* Compiler::visitTernOp(AST* a) {

}

MType* Compiler::visitForLoop(AST* a) {

}

MType* Compiler::visitWhileLoop(AST* a) {

}

MType* Compiler::visitDoWhile(AST* a) {

}

MType* Compiler::visitSwitch(AST* a) {

}

MType* Compiler::visitInterface(AST* a) {

}

MType* Compiler::visitClass(AST* a) {

}

MType* Compiler::visitNewClass(AST* a) {

}

MType* Compiler::visitArray(AST* a) {

}

MType* Compiler::visitNumber(AST* a) {
    auto tmp = ((Number*)a)->number;
    if (tmp.find(".") != std::string::npos) {
        loadConstS(pushConstL(MicaValue::Float(std::stof(tmp))));
        return new BaseType(BaseType::MDOUBLE);
    }
    loadConstS(pushConstL(MicaValue::Int(stol(tmp))));
    return new BaseType(BaseType::MINT);
}

MType* Compiler::visitChar(AST* a) {
    loadConstS(pushConstL(MicaValue::Char(((Char*)a)->c)));
    return new BaseType(BaseType::MCHAR);
}

MType* Compiler::visitBool(AST* a) {
    loadConstS(pushConstL(MicaValue::Bool(((Bool*)a)->bol == "true")));
    return new BaseType(BaseType::MBOOL);
}

void Compiler::emit(std::string a, ByteCode b, ByteCode c, ByteCode d) {
    getCurrentTsk()->emit(a, b, c, d);
}

int Compiler::pushConstL(MicaValue v) {
    return getCurrentTsk()->pushLocalConst(v);
}

int Compiler::pushConstG(MicaValue v) {
    env->globalConstPool.push_back(v);
    return  env->globalConstPool.size()-1;
}

void Compiler::loadConstS(int addr) {
    emit(getLabel(), LOAD_SCST, addr, 0);
}

void Compiler::loadConstG(int addr) {
    emit(getLabel(), LOAD_GCST, addr, 0);
}

std::string Compiler::getLabel() {
    return getCurrentTsk()->getLabel();
}

void Compiler::endTask() {

}

MType *Compiler::visitReturn(AST *) {
    return nullptr;
}

MType *Compiler::visitFuncTag(AST *) {
    return nullptr;
}
