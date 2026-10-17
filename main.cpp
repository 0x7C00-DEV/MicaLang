#include <iostream>
#include "fstream"
#include "include/lexer.h"
#include "include/makeError.h"
#include "include/symbol.h"
#include "include/dis.h"
#include "include/repl.h"
#include "include/loader.h"
#include "include/parser.h"
#include "include/asm.h"
#include "include/vm.h"
#include "include/compiler.h"
#include "include/debug.h"

#pragma comment(lib, "imagehlp.lib")

std::string getFileName(std::string name) {
    return name.substr(0, name.rfind(".")) + ".cm";
}

std::string loadFile(std::string path) {
    std::ifstream ifs(path);
    std::string res, buffer;
    while (std::getline(ifs, buffer))
        res += buffer + '\n';
    return res;
}

void testLexer() {
    Lexer lexer;
    while (true) {
        printf(">>> ");
        std::string expr;
        std::getline(std::cin, expr);
        lexer.resetExpr(expr, "<stdin>");
        Token current = lexer.getToken(SYN_VALUE);
        while (current.kind != TT_EOF) {
            current.debug();
            current = lexer.getToken(SYN_VALUE);
        }
    }
}

void testFile() {
    std::string file = R"(C:\Users\Lenovo\Desktop\MicaLang\test\example.mica)";
    CompileEnvironment* ce = new CompileEnvironment;
    ErrorCollect* ec = new ErrorCollect(nullptr);
    Compiler compiler(ce, ec);
    Module* mod = compiler.getProgram(loadFile(file), file, ec);
    printf("begin:\n");
    VM vm(mod);
    printf("end\n");
}

int main(int argc, char** argv) {
    Repl rpl(new ErrorCollect(nullptr));
    rpl.run();
    return 0;
}
