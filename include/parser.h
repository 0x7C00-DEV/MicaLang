//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_PARSER_H
#define MICALANG_PARSER_H
#define test(x) if (!x.isSuc) {\
                    return x; \
                }
#define setError(x, cnd, info) if (!(cnd)) {\
                        x.fai(info, current.begin, current.end);\
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
    Position begin, end;

    Register();
    Register(std::string, Position, Position);
    Register(AST*);

    void ok(AST*);
    void fai(std::string, Position, Position);
};

class TRegister {
public:
    bool isSuc;
    std::vector<AST*> result;
    std::string error;
    Position begin, end;

    TRegister();
    TRegister(std::string, Position, Position);
    TRegister(std::vector<AST*>);

    void ok(std::vector<AST*>);
    void fai(std::string, Position, Position);
};

class Parser;

using MPCLBCK = Register(Parser::*)();

class Parser {
public:
    Parser();
    Register parseExpr(std::string, std::string);
    std::vector<Register> parseCode(std::string, std::string);
private:
    Token current;
    Lexer lexer;
    std::vector<Token> states;
    void saveState();
    void restore();
    void advance(int cs = SYN_VALUE);
    bool equal(TokenKind);
    bool equal(std::string);
    Position posBegin();
    Position posEnd();

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
    TRegister makeTemplate();

    Register makeForInit();
    Register makeForChange();

    Register makeVarDefGrp();
    Register makeVarDefine();
    Register makeElementGetN(AST*, int);
    Register makeCallNodeN(AST*, std::vector<AST*>, int);
    Register makeElementGet(int);
    Register makeCallNode();
    Register makeMemberAccess(int);
    Register makeMemberAccessN(AST*, std::vector<AST*>, int);
    Register makeId(int);

    Register makeIf();
    Register makeFor();
    Register makeWhile();
    Register makeDoWhile();
    Register makeSwitch();
    Register makeBlock();
    Register makeStmt();
    Register makeCase();
    Register makeInterface();
    Register makeFunctionTag(AccessType);
    Register makeFunction();
};

#endif //MICALANG_PARSER_H