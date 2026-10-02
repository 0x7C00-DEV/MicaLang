#include <iostream>
#include "fstream"
#include "include/lexer.h"
#include "include/makeError.h"
#include "include/symbol.h"
#include "include/dis.h"
#include "include/parser.h"
#include "include/asm.h"
#include "include/vm.h"
#include "include/compiler.h"
#include "include/debug.h"

#pragma comment(lib, "imagehlp.lib")

std::string loadFile(std::string path) {
    std::ifstream ifs(path);
    std::string res, buffer;
    while (std::getline(ifs, buffer))
        res += buffer + '\n';
    return res;
}

void shell() {
    Parser p;
    while (true) {
        printf(">>> ");
        std::string expr;
        std::getline(std::cin, expr);
        auto tmp = p.parseExpr(expr, "<stdin>");
        if (!tmp.isSuc) {
            std::cout << "Error: " << tmp.error << std::endl;
        } else {
            showAST(tmp.result, 0, "", "\n");
        }
    }
}

void testFile() {
    std::string file = R"(C:\Users\Lenovo\Desktop\MicaLang\test\example.mica)";
    CompileEnvironment* ce = new CompileEnvironment;
    Compiler compiler(ce);
    Module* mod = compiler.getProgram(loadFile(file), file);
    Dis dis(mod);
    dis.disAll();
    printf("START_RUNNING:\n");
    VM vm(mod);
    printf("END.\n");
}

void release(int argc, char** argv) {
    std::string file = argv[1];
    CompileEnvironment* ce = new CompileEnvironment;
    Compiler compiler(ce);
    Module* mod = compiler.getProgram(loadFile(file), file);
    Dis dis(mod);
    dis.disAll();
    printf("START_RUNNING:\n");
    VM vm(mod);
    printf("END.\nVARS:");
    vm.dumpGlobalVars();
}

int main(int argc, char** argv) {
    testFile();
    return 0;
}
