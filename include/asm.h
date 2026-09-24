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
    
};

#endif 