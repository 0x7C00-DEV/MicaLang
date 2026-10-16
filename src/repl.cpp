#include "../include/repl.h"
#include <iostream>

Repl::Repl() {
    env_.moduleName = "__repl__";
    compiler_ = std::make_unique<Compiler>(&env_);

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

void Repl::run() {
    std::string line;
    std::cout << "Mica REPL\n";
    while (true) {
        std::cout << ">>> " << std::flush;
        if (!std::getline(std::cin, line)) { std::cout << "\n"; break; }
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        try { eval(line); }
        catch (...) { std::cout << "error\n"; }
    }
}

void Repl::eval(const std::string& line) {
    counter_++;
    std::string filename = "<repl:" + std::to_string(counter_) + ">";

    Module* fresh = compiler_->getProgram(line, filename);
    if (!fresh) { std::cout << "compile failed\n"; return; }

    std::vector<MicaValue> oldGlobals = module_->globalVars;
    module_->globalConstPool = fresh->globalConstPool;
    module_->globalVars      = oldGlobals;
    module_->isReady         = true;

    for (auto& v : module_->globalConstPool) {
        if (v.kind == MicaValue::OBJ && v.obj->tp == Obj::FUNCTION)
            ((Function*)v.obj)->module = module_;
    }

    vm_->env.mainModule = module_;
    vm_->env.modules.clear();
    vm_->env.modules.push_back(module_);

    int base = (int)vm_->env.callChain.size();
    vm_->start0("__repl__", "@init", {});
    vm_->executeLoop(base);
}