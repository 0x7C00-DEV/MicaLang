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
    if (a->kind == AST::AST_ID)
        return visitId(a);
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
    if (a->kind == AST::AST_NEG)         return visitNeg(a);
    if (a->kind == AST::AST_BIT_NOT)     return visitBitNot(a);
    if (a->kind == AST::AST_NOT)         return visitNot(a);
    if (a->kind == AST::AST_SELF_CHANGE) return visitSelfChange(a);
    std::cout << "not suppose\n";
    return nullptr;
}

MType* Compiler::visitAssign(AST* a) {

}

MType* Compiler::visitStmt(AST* a, std::string begin, std::string end) {
    if (a->kind == AST::AST_FUNC_DEF)
        return visitFunction(a);
    if (a->kind == AST::AST_IF)
        return visitIf(a, begin, end);
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
        return visitInterface(a);
    if (a->kind == AST::AST_FUNC_TAG)
        return visitFuncTag(a);
    return visitValue(a);
}

MType* Compiler::visitTernOp(AST* a) {

}

MType* Compiler::visitForLoop(AST* a) {
    auto tmp = (ForLoop*)a;
    std::string begin = getLabel();
    std::string changeLabel = getLabel();
    std::string end = getLabel();
    visitValue(tmp->init);
    emit(begin, NOP, 0, 0);
    MType* tp = nullptr;
    if (tmp->condition) {
        tp = visitValue(tmp->condition);
        emit("", JMPF, end, 0);
    }
    visitBlock(tmp->block, changeLabel, end);
    emit(changeLabel, NOP, 0, 0);
    visitValue(tmp->change);
    emit("", JMP, begin, 0);
    emit(end, NOP, 0, 0);

    if (tp && tp->__str__() != "bool;") {
        std::cout << "WARN: want a bool, but get a '"
                  << tp->__str__() << "'\n";
    }
    return nullptr;
}

MType* Compiler::visitWhileLoop(AST* a) {
    auto tmp = (WhileLoop*) a;
    std::string begin = getLabel();
    std::string end = getLabel();
    emit(begin, NOP, 0, 0);
    auto tp = visitValue(tmp->condition);
    emit(getLabel(), JMPF, end, 0);
    visitBlock(tmp->body, begin, end);
    emit(getLabel(), JMP, begin, 0);
    emit(end, NOP, 0,0);
    if (tp->__str__() != "bool;")
        std::cout << "warn in visitWhile, want a bool value, but get a '" << tp->__str__() << "'\n";
    return nullptr;
}

MType* Compiler::visitDoWhile(AST* a) {
    auto tmp = (DoWhile*) a;
    std::string begin = getLabel();
    std::string end = getLabel();
    emit(begin, NOP, 0, 0);
    visitBlock(tmp->body, begin, end);
    auto tp = visitValue(tmp->condition);
    emit(getLabel(), JMPT, begin, 0);
    emit(end, NOP, 0, 0);
    return tp;
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

void Compiler::endTask(bool isNative) {
    auto tmp = getCurrentTsk()->getCompileResult();
    tmp->isNative = isNative;
    env->addFunctionValue(tmp);
    env->tasks.pop_back();
    env->cs.leaveScope();
}

MType *Compiler::visitReturn(AST *a) {
    auto tmp = (Return*)a;
    MType* tp = new BaseType(BaseType::MVOID);
    if (tmp->value) {
        tp = visitValue(tmp->value);
    } else {
        emit(getLabel(), LOAD_NULL, 0, 0);
    }
    emit(getLabel(), RET, 0, 0);
    return tp;
}

MType *Compiler::visitFuncTag(AST *) {
    return nullptr;
}

MType *Compiler::visitId(AST *a) {
    auto name = ((Id*)a)->name;
    auto info = getCurrentTsk()->currentScope->lookupSymbol(name);
    if (info->kind == Symbol::SYM_VAR) {
        emit(getLabel(), LOAD_SVAR, ((VarSymbol*)info)->id, 0);
        return ((VarSymbol*)info)->type;
    } else if (info->kind == Symbol::SYM_FUNC) {
        for (int i = 0; i < env->globalConstPool.size(); ++i) {
            if (env->globalConstPool[i].kind == MicaValue::OBJ && env->globalConstPool[i].obj->tp == Obj::FUNCTION && ((Function*)env->globalConstPool[i].obj)->name == name) {
                emit(getLabel(), LOAD_GCST, i, 0);
                return ((FunctionSymbol*)info)->type;
            }
        }
        return nullptr;
    } else if (info->kind == Symbol::SYM_MODULE) {
        std::cout << "not suppose\n";
        exit(-1);
    } else if (info->kind == Symbol::SYM_CLASS) {
        for (int i = 0; i < env->globalConstPool.size(); ++i) {
            if (env->globalConstPool[i].kind == MicaValue::OBJ && env->globalConstPool[i].obj->tp == Obj::USER_DEFING_CLASS && ((ObjClass*)env->globalConstPool[i].obj)->name == name) {
                emit(getLabel(), LOAD_GCST, i, 0);
                return new ClassType(name, info);
            }
        }
    }
    std::cout << "not suppose\n";
    return nullptr;
}

MType *Compiler::visitNeg(AST *a) {
    auto tmp = (Neg*)a;
    auto val = visitValue(tmp->value);
    emit(getLabel(), BNEG, 0 , 0);
    return val;
}

MType *Compiler::visitSelfChange(AST *) {
    return nullptr;
}

MType *Compiler::visitBitNot(AST *pAst) {
    auto bn = (BitNot*) pAst;
    auto tmp = visitValue(bn->value);
    emit(getLabel(), BBNOT, 0, 0);
    return tmp;
}

MType *Compiler::visitNot(AST *pAst) {
    auto bn = (Not*) pAst;
    auto tmp = visitValue(bn->value);
    emit(getLabel(), BNOT, 0, 0);
    return tmp;
}

MType *Compiler::visitIf(AST *a, std::string begin, std::string end) {
    createScope(Scope::SNORMAL_BLOCK);
    auto tmp = (If*)a;
    std::string if_ = getLabel();
    std::string ie = getLabel();
    auto tp = visitValue(tmp->condition);
    if (tp->__str__() != "bool;")
        std::cout << "warn in visitIf, want a bool value, but get a '" << tp->__str__() << "'\n";
    emit(getLabel(), JMPF, if_, 0);
    visitBlock(tmp->tblock, begin, end);
    emit(getLabel(), JMP, ie, 0);
    emit(if_, NOP, 0, 0);
    visitBlock(tmp->fblock, begin, end);
    emit(ie, NOP, 0, 0);
    leaveScope();
    return nullptr;
}

MType *Compiler::visitBlock(AST *a, std::string begin, std::string end) {
    return nullptr;
}

int CompileEnvironment::addFunctionValue(Function *f) {
    globalConstPool.push_back(MicaValue::Object(f));
    return globalConstPool.size()-1;
}
