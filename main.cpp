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

int main() {
    Parser p;
    std::string expr = loadFile(R"(..\test\main.mic)");
    std::cout << expr << std::endl;
    auto tmp = p.parseExpr(expr);
    if (!tmp.isSuc) {
        std::cout << "ERROR: " << tmp.error << std::endl;
    } else {
        showAST(tmp.result, 0, "", "\n");
    }
    return 0;
}
