#ifndef ASM
#define ASM

struct Instr {
    int op;
    int v1;
    int v2;
};

Instr decodeInstr(int);

int codingInstr(Instr);

enum Assembly {
    BIN_OPER=1,
    JMP,
    JMPF,
    JMPT,
    CALL,
    LOAD_GVAR,
    STORE_GVAR,
    LOAD_SVAR,
    STORE_SVAR,
    LOAD_GCST,
    LOAD_SCST,
    IMM,
    POP,
    BNOT,
    BBNOT,
    BNEG,
    RET,
    NEW,
    DUP,
    MEM_GET,
    MEM_SET,
    EL_GET,
    EL_SET,
    NEW_ARR
};

enum BINOP {
    BADD=1, BSUB, BDIV, BMUL,
    BSHL, BSHR, BBAND, BBOR, BXOR,
    BMOD,
    BEQ, BNEQ, BAND, BOR, BEQORBIG, BEQORLESS, BBIG, BLESS
};

#endif 