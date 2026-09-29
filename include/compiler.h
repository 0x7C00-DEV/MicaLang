//
// Created by Lenovo on 2026/9/25.
//

#ifndef MICALANG_COMPILER_H
#define MICALANG_COMPILER_H
#include "../include/compiler.h"
#include "../include/symbol.h"
#include "../include/value.h"
#include "../include/parser.h"
#include "../include/asm.h"

struct ByteCode {
    std::string label;
    int opera=0;
    bool isLabel=false;
    ByteCode(std::string);
    ByteCode(int);
};

struct Instruction {
    std::string index;
    ByteCode a, b, c;
    Instruction(std::string, ByteCode, ByteCode, ByteCode);
    int toIns();
};


struct CurrentCompileTask {
    std::string funcName;
    std::vector<Instruction> ins;
    std::vector<MicaValue> localConstPool;
    int localVarCnt=0;
    int localLabelCnt=0;
    int pushLocalConst(MicaValue);
    int addLocalVar(std::string, Symbol*);
    Symbol* lookupLocalVar(std::string);
    Scope* functionScope;
    Scope* currentScope;
    std::string getLabel();
    CurrentCompileTask(Scope*, std::string);
    void fullBackLabel();
    void emit(std::string, ByteCode, ByteCode, ByteCode);
    Function* getCompileResult();
};

struct CompileEnvironment {
    CodeStruct cs;
    std::vector<MicaValue> globalConstPool;
    std::vector<CurrentCompileTask*> tasks;
    int addFunctionValue(Function*);
};

class Compiler {
public:
    Compiler(CompileEnvironment*);
private:
	Parser parser;
    CompileEnvironment* env;
    std::unordered_map<std::string, int> opera;
    CurrentCompileTask* getCurrentTsk();
    Scope* createScope(Scope::ScopeKind);
    void leaveScope();
    void createTask(std::string);
    void endTask(bool);
    void emit(std::string, ByteCode, ByteCode, ByteCode);
    int pushConstL(MicaValue);
    int pushConstG(MicaValue);
    void loadConstS(int);
    void loadConstG(int);
    std::string getLabel();

    MType* visitBlock(AST*, std::string, std::string);
    MType* visitBinOpNode(AST*);
    MType* visitCallNode(AST*);
    MType* visitElementGet(AST*);
    MType* visitMemberAccess(AST*);
    MType* visitFunction(AST*);
    MType* visitValue(AST*);
    MType* visitAssign(AST*);
    MType* visitStmt(AST*, std::string, std::string);
    MType* visitFuncTag(AST*);
    MType* visitIf(AST*, std::string, std::string);
    MType* visitTernOp(AST*);
    MType* visitForLoop(AST*);
    MType* visitWhileLoop(AST*);
    MType* visitDoWhile(AST*);
    MType* visitSwitch(AST*);
    MType* visitInterface(AST*);
    MType* visitClass(AST*);
    MType* visitNewClass(AST*);
    MType* visitArray(AST*);
    MType* visitNumber(AST*);
    MType* visitChar(AST*);
    MType* visitBool(AST*);
    MType* visitReturn(AST*);
    MType* visitId(AST*);
    MType* visitNeg(AST*);
    MType* visitSelfChange(AST*);

    void storeArray(std::vector<AST*>);
    void storeString(std::string);
    MType *visitBitNot(AST *pAst);

    MType *visitNot(AST *pAst);
};

#endif //MICALANG_COMPILER_H
