//
// Created by Lenovo on 2026/9/24.
//
#include "../include/loader.h"
#include "../include/vm.h"

static double asDouble(const MicaValue& v) {
    return (v.kind == MicaValue::INT) ? (double)v.i : v.f;
}

Function* VM::lookFunction(std::string name) {
    for (auto i : env.mainModule->globalConstPool)
        if (i.kind == MicaValue::OBJ && i.obj->tp == Obj::FUNCTION && ((Function*)i.obj)->name == name)
            return (Function*)i.obj;
    return nullptr;
}

VM::VM(Module* module) {
    initVec();
#ifdef TEST
    registNativeFunction(module);
#endif
    start(module);
}

void VM::start(Module* module) {
    env.modules.push_back(module);
    env.mainModule = module;

    for (auto& i : module->globalConstPool) {
        if (i.kind != MicaValue::OBJ) continue;

        if (i.obj->tp == Obj::FUNCTION) {
            ((Function*)i.obj)->module = module;
        } else if (i.obj->tp == Obj::USER_DEFING_CLASS) {
            auto cls = (ObjClass*)i.obj;
            for (auto& [name, mv] : cls->methods)
                if (mv.kind == MicaValue::OBJ && mv.obj->tp == Obj::FUNCTION)
                    ((Function*)mv.obj)->module = module;
        }
    }

    if (!module->isReady)
        initModule(module);

    auto mainFn = lookFunction("main");
    if (!mainFn) {
        std::cout << "Function 'main' not found in module '"
                  << module->moduleName << "'\n";
        exit(-1);
    }
    int base = env.callChain.size();
    callFunction(mainFn, {});
    executeLoop(base);
}

void VM::executeLoop(int base) {
    while (env.callChain.size() > base) {
        execute(env.getCTask()->getInstr());
    }
}

void VM::initModule(Module* module) {
    module->isReady = true;
#ifdef TEST
    registNativeFunction(module);
#endif
    auto base = env.callChain.size();
    start0(module->moduleName, "@init", {});
    executeLoop(base);
}

void VM::importModule(int a, int b) {
    auto modulePath_ = loadSubConst(a);
    auto moduleAlign_ = loadSubConst(b);
    std::string path, align;
    for (auto i : ((ObjArray*)modulePath_.obj)->elements)
        path += i.c;
    for (auto i : ((ObjArray*)moduleAlign_.obj)->elements)
        align += i.c;
    ProgramLoader loader(path);
    auto p = loader.getData();
    env.loadModule(this, p, align, path);
}

void VM::initVec() {
    IVEC[BIN_OPER] = &VM::binOp;
    IVEC[JMP] = &VM::jmp;
    IVEC[JMPF] = &VM::jmpf;
    IVEC[JMPT] = &VM::jmpt;
    IVEC[SWAP_SP] = &VM::swapSp;
    IVEC[CALL] = &VM::call;
    IVEC[IMPORT_MODULE] = &VM::importModule;
    IVEC[LOAD_GVAR] = &VM::loadGVar;
    IVEC[STORE_GVAR] = &VM::storeGVar;
    IVEC[LOAD_SVAR] = &VM::loadSVar;
    IVEC[SWAP] = &VM::swap;
    IVEC[STORE_SVAR] = &VM::storeSVar;
    IVEC[LOAD_MODULE] = &VM::loadModule;
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
    IVEC[NOP] = &VM::nop;
    IVEC[EL_SET] = &VM::elSet;
    IVEC[LOAD_TRUE] = &VM::loadTrue;
    IVEC[LOAD_FALSE] = &VM::loadFalse;
    IVEC[LOAD_NULL] = &VM::loadNull;
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

#ifdef TEST
void VM::registNativeFunction(Module* mod) {
    auto tmp = getFuncs();
    for (auto i : tmp) {
        bool exists = false;
        for (auto& v : mod->globalConstPool) {
            if (v.kind == MicaValue::OBJ && v.obj->tp == Obj::FUNCTION &&
                ((Function*)v.obj)->name == i->name) {
                if (!((Function*)v.obj)->isNative) {
                    ((Function*)v.obj)->__native__ = i->nativeFn->__native__;
                    ((Function*)v.obj)->isNative  = true;
                }
                exists = true;
                break;
                }
        }
        if (!exists)
            mod->globalConstPool.push_back(MicaValue::Object(i->nativeFn));
    }
}
#endif

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

VM::VM() {
    initVec();
}

VM::VM(std::string moduleName) {
    initVec();
    env.mainModule = new Module;
    env.mainModule->moduleName = moduleName;
    env.modules.push_back(env.mainModule);
#ifdef TEST
    registNativeFunction(env.mainModule);
#endif
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

void VM::swapSp(int, int) {
    auto sp = pop();
    auto und = pop();
    push(sp);
    push(und);
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

void VM::start0(std::string moduleName, std::string funcName, std::vector<MicaValue> args) {
    bool mf=false;
    for (auto i : env.modules)
        if (i->moduleName == moduleName) {
            mf=true;
            for (auto j : i->globalConstPool)
                if (j.kind == MicaValue::OBJ && j.obj->tp == Obj::FUNCTION && ((Function*)j.obj)->name == funcName) {
                    callFunction((Function*)j.obj, args);
                    return;
                }
        }
    if (!mf) {
        std::cout << "Module '" << moduleName << "' not found\n";
        exit(-1);
    }
    std::cout << "Function '" << funcName << "' not found\n";
    exit(-1);
}

void VM::start0(std::string path) {
    ProgramLoader loader(path);
    env.mainModule = env.loadModule(this, loader.getData(), "Main", path);
#ifdef TEST
    registNativeFunction(env.mainModule);
#endif
    int base = env.callChain.size();
    start0("Main", "main", {});
    executeLoop(base);
}

void VM::loadModule(int a, int b) {
    auto module_ = loadSubConst(a);
    std::string name ;
    for (auto i : ((ObjArray*)module_.obj)->elements)
        name += i.c;
    for (auto i : env.modules)
        if (i->moduleName == name) {
            push(MicaValue::Object(i));
            return;
        }
    std::cout << "module '" << name << "' not found.\n";
    exit(-1);
}

void VM::loadModuleMember(int a , int b) {
    auto module = (Module*) pop().obj;
    auto name = loadSubConst(a);
    std::string n;
    for (auto i : ((ObjArray*)name.obj)->elements)
        n += i.c;
    for (auto i : module->globalConstPool) {
        if (i.kind == MicaValue::OBJ && i.obj->tp == Obj::FUNCTION && ((Function*)i.obj)->name == n) {
            push(i);
            return;
        }
        if (i.kind == MicaValue::OBJ && i.obj->tp == Obj::USER_DEFING_CLASS && ((ObjClass *) i.obj)->name == n) {
            push(i);
            return;
        }
    }
    std::cout << "Member '" << n << "not found in module '" << module->moduleName << "' \n";
    exit(-1);
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
    if (v.kind != MicaValue::MBOOL) {
        std::cerr << "Not a bool value.\n";
        exit(-1);
    }
    if (!v.b) env.getCTask()->pc = a;
}

void VM::jmpt(int a, int b) {
    auto v = pop();
    if (v.kind != MicaValue::MBOOL) {
        std::cerr << "Not a bool value.\n";
        exit(-1);
    }
    if (v.b) env.getCTask()->pc = a;
}

void VM::swap(int a, int b) {
    auto i1 = (int)env.getCTask()->mstack.size() - 1 - a;
    auto i2 = (int)env.getCTask()->mstack.size() - 1 - b;
    std::swap(env.getCTask()->mstack[i1], env.getCTask()->mstack[i2]);
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
        std::cout << (tmp.obj)->tp << " not a function.\n";
        exit(-1);
    }
    auto fn = (Function*) tmp.obj;
    callFunction(fn, args);
}

void VM::loadGVar(int a, int b) {
    push(accessGlobalVar(a));
}

void VM::storeGVar(int a, int b) {
    setGlobalVar(a);
}

void VM::loadSVar(int a, int b) {
    push(accessSubVar(a));
}

void VM::storeSVar(int a, int b) {
    setSubVar(a);
}

void VM::loadGCst(int a, int b) {
    push(loadGlobalConst(a));
}

void VM::loadSCst(int a, int b) {
    push(loadSubConst(a));
}

void VM::imm(int a, int b) {
    push(MicaValue::Int(a));
}

void VM::pop_(int a, int b) {
    pop();
}

void VM::bnot(int a, int b) {
    auto tmp = pop();
    if (tmp.kind != MicaValue::MBOOL) {
        std::cout << "Not a boolean\n";
        exit(-1);
    }
    push(MicaValue::Bool(!tmp.b));
}

void VM::bitnot(int a, int b) {
    auto tmp = pop();
    if (tmp.kind != MicaValue::INT) {
        std::cout << "Not a integer\n";
        exit(-1);
    }
    push(MicaValue::Int(~tmp.i));
}

void VM::neg(int a, int b) {
    auto tmp = pop();
    if (tmp.kind == MicaValue::FLOAT) {
        push(MicaValue::Float(-tmp.f));
        return;
    }
    if (tmp.kind == MicaValue::INT) {
        push(MicaValue::Int(-tmp.i));
        return;
    }
    std::cerr << "unsuppose operator symbol '-'\n";
    exit(-1);
}

void VM::ret(int a, int b) {
    auto retVal = pop();
    Frame* cur = env.callChain.back();
    if (cur->caller) cur->caller->mstack.push_back(retVal);
    env.callChain.pop_back();
}

void VM::newObj(int a, int b) {
    ObjClass* cls = (ObjClass*) pop().obj;
    ObjInstance* ins = new ObjInstance();
    env.addObject(ins);
    ins->cls = cls;
    for (auto i : cls->fields)
        ins->fields[i] = MicaValue::Null();
    push(MicaValue::Object(ins));
}

void VM::dup(int a, int b) {
    push(env.getCTask()->mstack[env.getCTask()->mstack.size()-1-a]);
}

void VM::memGet(int a, int b) {
    std::string fieldName;
    // [obj, fieldName]
    auto fieName = pop();
    auto arr = (ObjArray*)(((ObjInstance*)fieName.obj)->cls);
    for (auto i : arr->elements)
        fieldName += i.c;
    auto obj = pop();
    if (obj.obj->tp == Obj::USER_DEFING_CLASS) {
        push(*((ObjClass*)obj.obj)->findMethod(fieldName));
        return;
    }
    push(((ObjInstance*)obj.obj)->getField(fieldName));
}

void VM::memSet(int a, int b) {
    std::string fieldName;
    auto fieName = pop();
    auto arr = (ObjArray*)(((ObjInstance*)fieName.obj)->cls);
    for (auto i : arr->elements)
        fieldName += i.c;
    auto value = pop();
    auto obj = pop();
    ((ObjInstance*)obj.obj)->setField(fieldName, value);
}

void VM::elGet(int a, int b) {
    auto pos = pop();
    auto obj = pop();
    auto tmp = (ObjArray*)(((ObjInstance*)obj.obj)->cls);
    push(tmp->elements[pos.i]);
}

void VM::elSet(int a, int b) {
    // STACK: [obj, val, pos]
    auto pos = pop();
    auto val = pop();
    auto obj = pop();
    ((ObjArray*)(((ObjInstance*)obj.obj)->cls))->elements[pos.i] = val;
}

void VM::newArr(int a, int b) {
    auto size = pop();
    auto cls = new ObjArray();
    cls->elements.resize(size.i>0? size.i : 100);
    auto tmp = new ObjInstance();
    tmp->cls = cls;
    env.addObject(tmp);
    push(MicaValue::Object(tmp));
}

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
    if (left.kind == MicaValue::MBOOL && right.kind == MicaValue::MBOOL) {
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
    if (left.kind == MicaValue::MBOOL && right.kind == MicaValue::MBOOL) {
        return MicaValue::Bool(left.b != right.b);
    }
    std::cerr << "unsupported operand type(s) for !=\n";
    exit(-1);
}

MicaValue VM::BAND__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::MBOOL || right.kind != MicaValue::MBOOL) {
        std::cerr << "unsupported operand type(s) for &&\n";
        exit(-1);
    }
    return MicaValue::Bool(left.b && right.b);
}

MicaValue VM::BOR__(MicaValue left, MicaValue right) {
    if (left.kind != MicaValue::MBOOL || right.kind != MicaValue::MBOOL) {
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

void VM::loadNull(int, int) {
    push(MicaValue::Null());
}

void VM::loadTrue(int, int) {
    push(MicaValue::Bool(true));
}

void VM::loadFalse(int, int) {
    push(MicaValue::Bool(false));
}

void VM::nop(int, int) {}

void VM::dumpGlobalVars() {
    if (!env.mainModule) return;
    std::cout << "=== globalVars of '" << env.mainModule->moduleName << "' ===\n";
    for (int i = 0; i < (int)env.mainModule->globalVars.size(); ++i) {
        auto v = env.mainModule->globalVars[i];
        std::cout << "  [" << i << "] kind=" << v.kind;
        if (v.kind == MicaValue::OBJ && v.obj->tp == Obj::INITED_OBJECT) {
            auto ins = (ObjInstance*)v.obj;
            std::cout << " cls=" << ins->cls->name;
            if (ins->cls->name == "Array") {
                auto arr = (ObjArray*)ins->cls;
                std::cout << " = [";
                for (int k = 0; k < (int)arr->elements.size(); ++k) {
                    if (k) std::cout << ", ";
                    auto e = arr->elements[k];
                    if (e.kind == MicaValue::INT)       std::cout << e.i;
                    else if (e.kind == MicaValue::FLOAT) std::cout << e.f;
                    else if (e.kind == MicaValue::MBOOL) std::cout << (e.b ? "true" : "false");
                    else                                 std::cout << "?kind=" << e.kind;
                }
                std::cout << "]";
            }
        } else if (v.kind == MicaValue::INT) {
            std::cout << " = " << v.i;
        }
        std::cout << "\n";
    }
}
