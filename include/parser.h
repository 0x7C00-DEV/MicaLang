//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_PARSER_H
#define MICALANG_PARSER_H
#define test(x) if (!x.isSuc) {\
                    return x; \
                }
#define setError(x, cnd, info) if (!(cnd)) {\
                        x.fai(info, current.lin, current.col);\
                        return x;\
                    }
#include "lexer.h"
#include "token.h"
#include "algorithm"
#include "ast.h"

class Register {
public:
    bool isSuc;
    AST* result{};
    std::string error;
    int lin{}, col{};

    Register();
    Register(std::string, int, int);
    Register(AST*);

    void ok(AST*);
    void fai(std::string, int, int);
};

class Parser;

using MPCLBCK = Register(Parser::*)();

class Parser {
public:
    Parser();
    Register parseExpr(std::string);
private:
    Token current;
    Lexer lexer;
    std::vector<Token> states;
    void saveState();
    void restore();
    void advance(int);
    bool equal(TokenKind);
    bool equal(std::string);
    Register makeBinOpNode(MPCLBCK, std::vector<std::string>);
    Register makeNumberNode();
    Register makeString();
    Register makeChar();
    Register makeValue();
    Register makeFactor(); 
    Register makeTerm(); 
    Register makeExpr1();
    Register makeExpr();
    Register makeExprA();
    Register makeExpr_();
    Register makeArray();
    Register makeType();

    Register makeForInit();
    Register makeForChange();
    
    Register makeVarDefGrp();
    Register makeVarDefine();
    Register makeElementGetN(AST*, int);
    Register makeCallNodeN(AST*, int);
    Register makeElementGet(int);
    Register makeCallNode();
    Register makeMemberAccess(int);
    Register makeMemberAccessN(AST*, int);
    Register makeId(int);

    Register makeIf();
    Register makeFor();
    Register makeWhile();
    Register makeDoWhile();
    Register makeSwitch();
    Register makeBlock();
    Register makeStmt();
    Register makeCase();
    Register makeFunction();
};

#endif //MICALANG_PARSER_H
