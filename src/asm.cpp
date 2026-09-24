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
        | (oper.op << 26)
        | (oper.v1 << 13)
        | oper.v2;
    return code;
}