#include "../include/repl.h"
#include <iostream>

Repl::Repl(ErrorCollect* ec) {
    this->ec = ec;
    env_.moduleName = "__repl__";
    compiler_ = std::make_unique<Compiler>(&env_, ec);

    module_ = new Module();
    module_->moduleName = "__repl__";
    module_->isReady = true;

    vm_ = std::make_unique<VM>();
    vm_->env.mainModule = module_;
    vm_->env.modules.push_back(module_);
#ifdef TEST
    vm_->registNativeFunction(module_);
#endif
}

Repl::~Repl() = default;

void calc(const std::string& expr, int& bigBlockCnt, int& midBlockCnt) {
    bigBlockCnt = 0;
    midBlockCnt = 0;
    for (int i = 0; i < (int)expr.size(); ++i) {
        if (expr[i] == '\'') {
            ++i;
            while (i < (int)expr.size() && expr[i] != '\'') ++i;
            continue;
        }
        if (expr[i] == '"') {
            ++i;
            while (i < (int)expr.size() && expr[i] != '"') ++i;
            continue;
        }
        if (expr[i] == '(') ++midBlockCnt;
        else if (expr[i] == ')') { if (midBlockCnt > 0) --midBlockCnt; }
        else if (expr[i] == '{') ++bigBlockCnt;
        else if (expr[i] == '}') { if (bigBlockCnt > 0) --bigBlockCnt; }
    }
}

void Repl::run() {
    std::string line;
    std::cout << "Mica REPL\n";
    while (true) {
        std::cout << ">>> " << std::flush;
        if (!std::getline(std::cin, line)) { std::cout << "\n"; break; }
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        int bigBlockCnt = 0, midBlockCnt = 0;
        calc(line, bigBlockCnt, midBlockCnt);
        while (bigBlockCnt != 0 || midBlockCnt != 0) {
            printf("...");
            std::string tmp;
            if (!std::getline(std::cin, tmp)) return;
            line += "\n" + tmp;
            calc(line, bigBlockCnt, midBlockCnt);
        }
        
        try { eval(line); }
        catch (...) { std::cout << "error\n"; }
    }
}

void Repl::eval(const std::string& line) {
    counter_++;
    std::string filename = "<repl:" + std::to_string(counter_) + ">";

    size_t mark = ec->errors.size();
    Module* fresh = compiler_->getProgram(line, filename, ec);
    if (!fresh) {
        for (size_t i = mark; i < ec->errors.size(); ++i)
            std::cout << ec->errors[i]->errorName << ": "
                      << ec->errors[i]->errorInfo << "\n";
        return;
    }

    std::vector<MicaValue> oldGlobals = module_->globalVars;
    module_->globalConstPool = fresh->globalConstPool;
    module_->globalVars      = oldGlobals;
    module_->isReady         = true;

    for (auto& v : module_->globalConstPool) {
        if (v.kind != MicaValue::OBJ) continue;

        if (v.obj->tp == Obj::FUNCTION) {
            ((Function*)v.obj)->module = module_;
        } else if (v.obj->tp == Obj::USER_DEFING_CLASS) {
            auto cls = (ObjClass*)v.obj;
            for (auto& [name, mv] : cls->methods) {
                if (mv.kind == MicaValue::OBJ && mv.obj->tp == Obj::FUNCTION)
                    ((Function*)mv.obj)->module = module_;
            }
        }
    }

    vm_->env.mainModule = module_;
    vm_->env.modules.clear();
    vm_->env.modules.push_back(module_);

    int base = (int)vm_->env.callChain.size();
    vm_->start0("__repl__", "@init", {});
    vm_->executeLoop(base);
}