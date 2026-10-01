#ifndef MICALANG_DIS_H
#define MICALANG_DIS_H
#include "value.h"
#include "asm.h"
#include "unordered_map"

const std::unordered_map<int, std::string> operMap = {
        {BIN_OPER, "BIN_OPER"}, {JMP, "JMP"}, {JMPF, "JMPF"}, {JMPT, "JMPT"},
        {CALL, "CALL"},
        {LOAD_GVAR, "LOAD_GVAR"}, {STORE_GVAR, "STORE_GVAR"},
        {LOAD_SVAR, "LOAD_SVAR"}, {STORE_SVAR, "STORE_SVAR"},
        {LOAD_GCST, "LOAD_GCST"}, {LOAD_SCST, "LOAD_SCST"},
        {IMPORT_MODULE, "IMPORT_MODULE"}, {LOAD_MODULE, "LOAD_MODULE"},
        {LOAD_TRUE, "LOAD_TRUE"}, {LOAD_FALSE, "LOAD_FALSE"}, {LOAD_NULL, "LOAD_NULL"},
        {NOP, "NOP"}, {IMM, "IMM"}, {POP, "POP"},
        {BNOT, "BNOT"}, {BBNOT, "BBNOT"}, {BNEG, "BNEG"},
        {RET, "RET"}, {NEW, "NEW"}, {DUP, "DUP"},
        {MEM_GET, "MEM_GET"}, {MEM_SET, "MEM_SET"},
        {EL_GET, "EL_GET"}, {EL_SET, "EL_SET"},
        {LOAD_MODULE_MEMBER, "LOAD_MODULE_MEMBER"},
        {NEW_ARR, "NEW_ARR"}
};

const std::unordered_map<int, std::string> binOpMap = {
        {BADD, "BADD"}, {BSUB, "BSUB"}, {BDIV, "BDIV"}, {BMUL, "BMUL"},
        {BSHL, "BSHL"}, {BSHR, "BSHR"},
        {BBAND, "BBAND"}, {BBOR, "BBOR"}, {BXOR, "BXOR"}, {BMOD, "BMOD"},
        {BEQ, "BEQ"}, {BNEQ, "BNEQ"},
        {BAND, "BAND"}, {BOR, "BOR"},
        {BEQORBIG, "BEQORBIG"}, {BEQORLESS, "BEQORLESS"},
        {BBIG, "BBIG"}, {BLESS, "BLESS"}
};

std::string getOperator(int);
std::string getBinOper(int);

class Dis {
public:
    explicit Dis(Module*);

    void disFunction(Function*);

    void disAll();

    void disModule();

    void disGlobalConstPool();

    void disClass(ObjClass*);

private:
    Module* module;

    void showConstPool(Function*);
    void showInstruation(Function*);

    void showConstValue(const MicaValue&);
};
#endif //MICALANG_DIS_H