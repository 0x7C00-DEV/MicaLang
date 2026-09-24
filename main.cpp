#include <iostream>
#include "fstream"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/debug.h"

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

int main() {
    Parser p;
    std::vector<Register> codes = p.parseCode(loadFile(R"(../test/main.mic)"), R"(../test/main.mic)");
    for (auto i : codes){
        if (!i.isSuc) {
            std::cout << i.error << std::endl;
        } else 
            showAST(i.result, 0, "", "\n");
    }
    return 0;
}
