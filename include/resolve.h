//
// Created by Lenovo on 2026/9/27.
//

#ifndef MICALANG_RESOLVE_H
#define MICALANG_RESOLVE_H
#include "ast.h"

class ClassInfo {
    std::vector<std::string> templates;
    std::unordered_map<std::string, AST*> fieldTypes;
};

class Resolve {
public:
    Resolve();
    void parseStmt(AST*);
private:
    std::unordered_map<std::string, ClassInfo> clsInfo;
    std::unordered_map<std::string, AST*> funcInfo;
    AST* parseBinOp(AST*);
    AST* parseCall(AST*);
    AST* parseElementGet(AST*);
    AST* parseSelfChange(AST*);
    AST* parseMemberGet(AST*);
    void parseFunction(AST*);
    void parseBlock(AST*);
};

#endif //MICALANG_RESOLVE_H
