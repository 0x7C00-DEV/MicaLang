#ifndef MICALANG_REPL_H
#define MICALANG_REPL_H

#include "compiler.h"
#include "vm.h"
#include <memory>
#include <string>

class Repl {
public:
    Repl();
    ~Repl();
    void run();

private:
    void eval(const std::string& line);

    CompileEnvironment env_;
    std::unique_ptr<Compiler> compiler_;
    std::unique_ptr<VM> vm_;
    Module* module_;
    int counter_ = 0;
};

#endif