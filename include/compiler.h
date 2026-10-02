//
// Created by Lenovo on 2026/9/25.
//

#ifndef MICALANG_COMPILER_H
#define MICALANG_COMPILER_H
#include "../include/compiler.h"
#include "../include/symbol.h"
#include "../include/loader.h"
#include "../include/value.h"
#include "../include/parser.h"
#include "../include/config.h"
#include "../include/asm.h"

#ifdef TEST
#include "native.h"
#endif

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
    std::unordered_map<std::string, MType*> templateTypes;
    std::string funcName;
    std::vector<Instruction> ins;
    std::vector<MicaValue> localConstPool;
    int localVarCnt=0;
    int localLabelCnt=0;
    int pushLocalConst(MicaValue);
    int addLocalVar(std::string, MType*, bool);
    Symbol* lookupLocalVar(std::string);
    Scope* functionScope;
    Scope* currentScope;
    std::string getLabel();
    CurrentCompileTask(Scope*, std::string);
    void fullBackLabel();
    void emit(std::string, ByteCode, ByteCode, ByteCode);
    Function* getCompileResult();
    MType* fnRetTp;
};

struct CompileEnvironment {
    std::string moduleName;
    int classCnt=0, interfaceCnt=0;
    CodeStruct cs;
    std::vector<MicaValue> globalConstPool;
    std::vector<Module> modules;
    std::vector<CurrentCompileTask*> tasks;
    int addFunctionValue(Function*);
};

class AstToMtype {
public:
    AstToMtype(CompileEnvironment*);
    MType* getType(AST*);
private:
    CompileEnvironment* env;
    MType* parseIdNode(AST*);
    MType* parseArrayType(AST*);
    MType* parseFuncType(AST*);
    MType* parseTemplateType(AST*);

    ClassSymbol* findClass(std::string);

    MType* findClassMember(AST*);
};

struct Vresult {
    int id;
    MType* type;
    std::string name;
};

class Compiler {
public:
    Compiler(CompileEnvironment*);
    Module *getProgram(std::string, std::string);
private:
	Parser parser;
    CompileEnvironment* env;
    AstToMtype* conver;
    std::unordered_map<std::string, int> opera;
    CurrentCompileTask* getCurrentTsk();
    Scope* createScope(Scope::ScopeKind);
#ifdef TEST
    void registNativeFunction();
#endif
    void leaveScope();
    void createTask(std::string);
    void endTask(bool);
    void emit(std::string, ByteCode, ByteCode, ByteCode);
    int addGlobalVar(std::string, MType*, bool);
    int pushConstL(MicaValue);
    int pushConstG(MicaValue);
    void loadConstS(int);
    void loadConstG(int);
    std::string getLabel();

    ClassSymbol* getClassInfo(std::string);
    ObjClass* getClassObject(std::string);
    int getClassId(std::string);

    InterfaceSymbol* getInterface(std::string);
    int getInterfaceId(std::string);

    void declareGlobalVars(AST*);
    Function* makeFunction(AST*);
    MType* visitBlock(AST*, std::string, std::string);
    MType* visitModuleImport(AST*);
    MType* visitBinOpNode(AST*, MType*);
    MType* visitCallNode(AST*, MType*);
    MType* visitElementGet(AST*, MType*);
    MType* visitMemberAccess(AST*, MType*);
    void visitVarDefineGrp(AST*, bool);
    Vresult visitVarDefine(AST*, bool);
    MType* visitFunction(AST*, bool);
    MType* visitValue(AST*, MType*);
    MType* visitAssign(AST*);
    MType* visitStmt(AST*, std::string, std::string);
    MType* visitFuncTag(AST*);
    MType* visitIf(AST*, std::string, std::string);
    MType* visitTernOp(AST*, MType*);
    MType* visitForLoop(AST*);
    MType* visitWhileLoop(AST*);
    MType* visitDoWhile(AST*);
    MType* visitSwitch(AST*);
    MType* visitInterface(AST*);
    MType* visitClass(AST*);
    MType* visitNewClass(AST*, MType*);
    MType* visitArray(AST*, MType*);
    void   visitBreak(AST*, std::string, std::string);
    void   visitContinue(AST*, std::string, std::string);
    MType* visitNumber(AST*);
    MType* visitChar(AST*);
    MType* visitBool(AST*);
    MType* visitReturn(AST*);
    MType* visitId(AST*);
    MType* visitNeg(AST*, MType*);
    MType* visitSelfChange(AST*);

    void storeArray(std::vector<AST*>);
    void storeString(std::string);
    MType *visitBitNot(AST *, MType*);

    MType *visitNot(AST *, MType*);
};

#endif //MICALANG_COMPILER_H
