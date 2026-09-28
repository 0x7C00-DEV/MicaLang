#include "../include/asm.h"

Instr decodeInstr(int code) {
    // 6(Operator) + 13(Value1) + 13(Value2)
    Instr ins;
    ins.op = (code >> 26) & 0x3F;        
    ins.v1 = (code >> 13) & 0x1FFF;     
    ins.v2 =  code        & 0x1FFF;     
    return ins;
}

int codingInstr(Instr oper) {
    int code = 0;
    code = code 
        |  ((oper.op & 0x3F) << 26)
        | ((oper.v1 & 0x1FFF) << 13)
        | oper.v2 & 0x1FFF;
    return code;
}

int codingInstr(int op, int v1, int v2) {
    int code = 0;
    code = code
           |  ((op & 0x3F) << 26)
           | ((v1 & 0x1FFF) << 13)
           | v2 & 0x1FFF;
    return code;
}