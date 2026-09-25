//
// Created by Lenovo on 2026/9/24.
//
#include "../include/vm.h"

static double asDouble(const MicaValue& v) {
    return (v.kind == MicaValue::INT) ? (double)v.i : v.f;
}

Function* VM::lookFunction(std::string name) {
    for (auto i : env.mainModule->funcs)
        if (i->name == name)
            return i;
    return nullptr;
}

VM::VM(Module* module, std::string func, std::vector<MicaValue> args) {
    initVec();
    env.modules.push_back(module);
    env.mainModule = module;
    for (auto i : module->funcs)
        i->module = module;
    auto tmp = lookFunction(func);
    if (!tmp) {
        std::cout << "Function not found in module '" << module->moduleName << "'\n";
        exit(-1);
    }
    callFunction(tmp, args);
    executeLoop();
}

void VM::executeLoop() {
    while (!env.callChain.empty())
        execute(env.getCTask()->getInstr());
}


void VM::initVec() {
    IVEC[BIN_OPER] = &VM::binOp;
    IVEC[JMP] = &VM::jmp;
    IVEC[JMPF] = &VM::jmpf;
    IVEC[JMPT] = &VM::jmpt;
    IVEC[CALL] = &VM::call;
    IVEC[LOAD_GVAR] = &VM::loadGVar;
    IVEC[STORE_GVAR] = &VM::storeGVar;
    IVEC[LOAD_SVAR] = &VM::loadSVar;
    IVEC[STORE_SVAR] = &VM::storeSVar;
    IVEC[LOAD_GCST] = &VM::loadGCst;
    IVEC[LOAD_SCST] = &VM::loadSCst;
    IVEC[IMM] = &VM::imm;
    IVEC[POP] = &VM::pop_;
    IVEC[BNOT] = &VM::bnot;
    IVEC[BBNOT] = &VM::bitnot;
    IVEC[BNEG] = &VM::neg;
    IVEC[RET] = &VM::ret;
    IVEC[NEW] = &VM::newObj;
    IVEC[DUP] = &VM::dup;
    IVEC[MEM_GET] = &VM::memGet;
    IVEC[MEM_SET] = &VM::memSet;
    IVEC[EL_GET] = &VM::elGet;
    IVEC[EL_SET] = &VM::elSet;
    IVEC[NEW_ARR] = &VM::newArr;
    IVEC[LOAD_MODULE_MEMBER] = &VM::loadModuleMember;

    BINOP[BADD] = &VM::BADD__;
    BINOP[BSUB] = &VM::BSUB__;
    BINOP[BDIV] = &VM::BDIV__;
    BINOP[BMUL] = &VM::BMUL__;
    BINOP[BSHL] = &VM::BSHL__;
    BINOP[BSHR] = &VM::BSHR__;
    BINOP[BBAND] = &VM::BBAND__;
    BINOP[BBOR] = &VM::BBOR__;
    BINOP[BXOR] = &VM::BXOR__;
    BINOP[BMOD] = &VM::BMOD__;
    BINOP[BEQ] = &VM::BEQ__;
    BINOP[BNEQ] = &VM::BNEQ__;
    BINOP[BAND] = &VM::BAND__;
    BINOP[BOR] = &VM::BOR__;
    BINOP[BEQORBIG] = &VM::BEQORBIG__;
    BINOP[BEQORLESS] = &VM::BEQORLESS__;
    BINOP[BBIG] = &VM::BBIG__;
    BINOP[BLESS] = &VM::BLESS__;
}


void VM::setGlobalVar(int address) {
    auto val = pop();
    if (address >= env.getCTask()->module->globalVars.size())
        env.getCTask()->module->globalVars.resize(address+12);
    env.getCTask()->module->globalVars[address] = val;
}

void VM::setSubVar(int address) {
    auto val = pop();
    if (address >= env.getCTask()->localVar.size())
        env.getCTask()->localVar.resize(address+12);
    env.getCTask()->localVar[address] = val;
}

VM::VM(std::string moduleName) {
    initVec();
    env.mainModule = new Module;
    env.mainModule->moduleName = moduleName;
    env.modules.push_back(env.mainModule);
}

void VM::push(MicaValue value) {
    env.getCTask()->mstack.push_back(value);
}

MicaValue VM::pop() {
    if (env.getCTask()->mstack.empty()) {
        std::cout << "ERROR: pop from empty stack.\n";
        exit(-1);
    }
    auto tmp = env.getCTask()->mstack.back();
    env.getCTask()->mstack.pop_back();
    return tmp;
}

void VM::execute(int instr) {
    Instr i = decodeInstr(instr);
    auto command = IVEC[i.op];
    (this->*command)(i.v1, i.v2);
}


void VM::execute(Instr instr) {
    (this->*IVEC[instr.op])(instr.v1, instr.v2);
}

void VM::binOp(int a, int b) {
    auto r = pop();
    auto l = pop();
    push((this->*(BINOP[a]))(l, r));
}


void VM::loadModuleMember(int a , int b) {

}

void VM::callFunction(Function* func, std::vector<MicaValue> args) {
    auto f = new Frame(func, (env.callChain.empty())? nullptr : env.getCTask());
    if (!func->isNative) {
        f->localVar.resize(args.size()+10);
        for (int i=0; i<args.size(); ++i)
            f->localVar[i] = args[i];
        env.callChain.push_back(f);
    } else {
        f->__call__(&env, args);
    }
}

void VM::jmp(int a, int b) {
    env.getCTask()->pc = a;
}

void VM::jmpf(int a, int b) {
    auto v = pop();
    if (v.kind != MicaValue::BOOL) {
        std::cerr << "Not a bool value.\n";
        exit(-1);
    }
    if (!v.b) env.getCTask()->pc = a;
}

void VM::jmpt(int a, int b) {
    auto v = pop();
    if (v.kind != MicaValue::BOOL) {
        std::cerr << "Not a bool value.\n";
        exit(-1);
    }
    if (v.b) env.getCTask()->pc = a;
}

void VM::call(int a, int b) {
    // Stack: [fn, v1, v2, v3, v4, ..., vN]
    std::vector<MicaValue> args;
    for (int i=0; i<a; ++i)
        args.push_back(pop());
    std::reverse(args.begin(), args.end());
    auto tmp = pop();
    if (tmp.kind != MicaValue::OBJ) {
        std::cout << "Not object.\n";
        exit(-1);
    }
    if ((tmp.obj)->tp != Obj::FUNCTION) {
        std::cout << "Not a function.\n";
        exit(-1);
    }
    auto fn = (Function*) tmp.obj;
    callFunction(fn, args);
}

void VM::loadGVar(int a, int b) {}

void VM::storeGVar(int a, int b) {
    setGlobalVar(a);
}

void VM::loadSVar(int a, int b) {}

void VM::storeSVar(int a, int b) {
    setSubVar(a);
}

void VM::loadGCst(int a, int b) {}

void VM::loadSCst(int a, int b) {}

void VM::imm(int a, int b) {}

void VM::pop_(int a, int b) {}

void VM::bnot(int a, int b) {}

void VM::bitnot(int a, int b) {}

void VM::neg(int a, int b) {}

void VM::ret(int a, int b) {
    auto retVal = pop();
    Frame* cur = env.callChain.back();
    if (cur->caller) cur->caller->mstack.push_back(retVal);
    env.callChain.pop_back();
}

void VM::newObj(int a, int b) {}

void VM::dup(int a, int b) {}

void VM::memGet(int a, int b) {}

void VM::memSet(int a, int b) {}

void VM::elGet(int a, int b) {}

void VM::elSet(int a, int b) {}

void VM::newArr(int a, int b) {}


MicaValue VM::accessSubVar(int address /*模块内索引*/) {
    return env.getCTask()->localVar[address];
}

MicaValue VM::accessGlobalVar(int address/*模块内索引*/) {
    return env.getCTask()->module->globalVars[address];
}

MicaValue VM::loadGlobalConst(int address/*模块内索引*/) {
    return env.getCTask()->module->globalConstPool[address];
}

MicaValue VM::loadSubConst(int address/*模块内索引*/) {
    return env.getCTask()->fn->constants[address];
}

MicaValue VM::BADD__(MicaValue left, MicaValue right) {
    if (left.kind == MicaValue::INT && right.kind == MicaValue::INT) {
        return MicaValue::Int(left.i + right.i);
    }
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Float(asDouble(left) + asDouble(right));
    }
    std::cerr << "unsupported operand type(s) for +\n";
    exit(-1);
}

MicaValue VM::BSUB__(MicaValue left, MicaValue right) {
    if (left.kind == MicaValue::INT && right.kind == MicaValue::INT) {
        return MicaValue::Int(left.i - right.i);
    }
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Float(asDouble(left) - asDouble(right));
    }
    std::cerr << "unsupported operand type(s) for -\n";
    exit(-1);
}

MicaValue VM::BDIV__(MicaValue left, MicaValue right) {
    if (left.kind == MicaValue::INT && right.kind == MicaValue::INT) {
        if (right.i == 0) { std::cerr << "division by zero\n"; exit(-1); }
        return MicaValue::Int(left.i / right.i);
    }
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        double r = asDouble(right);
        if (r == 0.0) { std::cerr << "division by zero\n"; exit(-1); }
        return MicaValue::Float(asDouble(left) / r);
    }
    std::cerr << "unsupported operand type(s) for /\n";
    exit(-1);
}

MicaValue VM::BMUL__(MicaValue left, MicaValue right) {
    if (left.kind == MicaValue::INT && right.kind == MicaValue::INT) {
        return MicaValue::Int(left.i * right.i);
    }
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Float(asDouble(left) * asDouble(right));
    }
    std::cerr << "unsupported operand type(s) for *\n";
    exit(-1);
}

MicaValue VM::BSHL__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::INT || right.kind != MicaValue::INT) {
        std::cerr << "unsupported operand type(s) for <<\n";
        exit(-1);
    }
    return MicaValue::Int(left.i << right.i);
}

MicaValue VM::BSHR__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::INT || right.kind != MicaValue::INT) {
        std::cerr << "unsupported operand type(s) for >>\n";
        exit(-1);
    }
    return MicaValue::Int(left.i >> right.i);
}

MicaValue VM::BBAND__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::INT || right.kind != MicaValue::INT) {
        std::cerr << "unsupported operand type(s) for &\n";
        exit(-1);
    }
    return MicaValue::Int(left.i & right.i);
}

MicaValue VM::BBOR__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::INT || right.kind != MicaValue::INT) {
        std::cerr << "unsupported operand type(s) for |\n";
        exit(-1);
    }
    return MicaValue::Int(left.i | right.i);
}

MicaValue VM::BXOR__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::INT || right.kind != MicaValue::INT) {
        std::cerr << "unsupported operand type(s) for ^\n";
        exit(-1);
    }
    return MicaValue::Int(left.i ^ right.i);
}

MicaValue VM::BMOD__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::INT || right.kind != MicaValue::INT) {
        std::cerr << "unsupported operand type(s) for %\n";
        exit(-1);
    }
    if (right.i == 0) { std::cerr << "modulo by zero\n"; exit(-1); }
    return MicaValue::Int(left.i % right.i);
}

MicaValue VM::BEQ__(MicaValue left, MicaValue right) {
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Bool(asDouble(left) == asDouble(right));
    }
    if (left.kind == MicaValue::BOOL && right.kind == MicaValue::BOOL) {
        return MicaValue::Bool(left.b == right.b);
    }
    std::cerr << "unsupported operand type(s) for ==\n";
    exit(-1);
}

MicaValue VM::BNEQ__(MicaValue left, MicaValue right) {
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Bool(asDouble(left) != asDouble(right));
    }
    if (left.kind == MicaValue::BOOL && right.kind == MicaValue::BOOL) {
        return MicaValue::Bool(left.b != right.b);
    }
    std::cerr << "unsupported operand type(s) for !=\n";
    exit(-1);
}

MicaValue VM::BAND__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::BOOL || right.kind != MicaValue::BOOL) {
        std::cerr << "unsupported operand type(s) for &&\n";
        exit(-1);
    }
    return MicaValue::Bool(left.b && right.b);
}

MicaValue VM::BOR__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::BOOL || right.kind != MicaValue::BOOL) {
        std::cerr << "unsupported operand type(s) for ||\n";
        exit(-1);
    }
    return MicaValue::Bool(left.b || right.b);
}

MicaValue VM::BEQORBIG__(MicaValue left, MicaValue right) {
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Bool(asDouble(left) >= asDouble(right));
    }
    std::cerr << "unsupported operand type(s) for >=\n";
    exit(-1);
}

MicaValue VM::BEQORLESS__(MicaValue left, MicaValue right) {
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Bool(asDouble(left) <= asDouble(right));
    }
    std::cerr << "unsupported operand type(s) for <=\n";
    exit(-1);
}

MicaValue VM::BBIG__(MicaValue left, MicaValue right) {
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Bool(asDouble(left) > asDouble(right));
    }
    std::cerr << "unsupported operand type(s) for >\n";
    exit(-1);
}

MicaValue VM::BLESS__(MicaValue left, MicaValue right) {
    if ((left.kind == MicaValue::INT || left.kind == MicaValue::FLOAT) &&
        (right.kind == MicaValue::INT || right.kind == MicaValue::FLOAT)) {
        return MicaValue::Bool(asDouble(left) < asDouble(right));
    }
    std::cerr << "unsupported operand type(s) for <\n";
    exit(-1);
}