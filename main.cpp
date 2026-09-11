#include <iostream>
#include "include/lexer.h"
#include "include/parser.h"
#include "include/debug.h"

int main() {
    Parser parser;
    while (true) {
        std::string expr;
        std::cout << ">>> ";
        std::getline(std::cin, expr);
        Register res = parser.parseExpr(expr);
        if (!res.isSuc) {
            std::cout << res.error << " at lin " << res.lin << ", col " << res.col << std::endl;
            continue;
        }
        showAST(res.result, 0, "", "\n");
    }
    return 0;
}
