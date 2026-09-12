//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_AST_H
#define MICALANG_AST_H
#include <complex>
#include <string>
#include <utility>
#include <vector>

struct AST {
    int lin, col;
    enum TKind {
        AST_BIN_OP, AST_DIGIT, AST_CHAR, AST_ARRAY, AST_NEG, AST_ELEMENT_GET, AST_CALL,
        AST_MEMBER_ACCESS, AST_ID, AST_BOOL, AST_NULL, AST_THREE_OP, AST_SELF_CHANGE,
        AST_ASSIGN_NODE, AST_RETURN, AST_CONTINUE, AST_BREAK, AST_GOTO, AST_BLOCK,
        AST_FOR, AST_WHILE, AST_DO_WHILE, AST_SWITCH, AST_TYPE, AST_VAR_DEF, AST_VAR_DEF_GRP,
        AST_CASE, AST_LABEL, AST_IF, AST_FUNC_DEF
    } kind;

    explicit AST(TKind kind, int lin, int col) {
        this->kind = kind;
        this->lin = lin;
        this->col = col;
    }
};

struct Func : AST {
    std::string name;
    AST* body;
    std::vector<AST*> args;
    bool isNative;
    AST* ftype;
    Func(std::string name, AST* body, std::vector<AST*> args, AST* ftype, bool isNative, int lin, int col) : AST(AST_FUNC_DEF, lin, col) {
        this->name = name;
        this->body = body;
        this->args = args;
        this->ftype = ftype;
        this->isNative = isNative;
    }
};

struct If : AST {
    AST* condition;
    AST* tblock;
    AST* fblock;
    If(AST* condition, AST* tblock, AST* fblock, int lin, int col): AST(AST_IF, lin, col) {
        this->condition = condition;
        this->tblock = tblock;
        this->fblock = fblock;
    }
};

struct Case : AST {
    AST* value;
    AST* block;
    Case(AST* value, AST* block, int lin, int col): AST(AST_CASE, lin, col) {
        this->value = value;
        this->block = block;
    }
};

struct Switch : AST {
    AST* value;
    std::vector<AST*> cases;
    Switch(AST* value, std::vector<AST*> cases, int lin, int col): AST(AST_SWITCH, lin, col) {
        this->value = value;
        this->cases = cases;
    }
};

struct Label : AST {
    AST* name;
    Label(AST* name, int lin, int col): AST(AST_LABEL, lin, col) {
        this->name = name;
    }
};

struct VarDefGrp : AST {
    std::vector<AST*> vars;
    VarDefGrp(std::vector<AST*> vars, int lin, int col): AST(AST_VAR_DEF_GRP, lin, col) {
        this->vars = vars;
    }
};

struct VarDef : AST {
    std::string name;
    AST* type;
    AST* init;
    VarDef(std::string name, AST* type, AST* init, int lin, int col): AST(AST_VAR_DEF, lin, col) {
        this->name = name;
        this->type = type;
        this->init = init;
    }
};

struct Type : AST {
    enum TPKind { TYPE_ARRAY, TYPE_TEMPLATE, TYPE_NORMAL, TYPE_FUNC } tpKind;
    Type(TPKind tp_kind, int lin, int col) : AST(AST_TYPE, lin, col), tpKind(tp_kind) {}
};

struct FuncType : Type {
    AST* retType;
    std::vector<AST*> args;
    FuncType(AST* retType, std::vector<AST*> args, int lin, int col): Type(TYPE_FUNC, lin, col) {
        this->retType = retType;
        this->args = args;
    }
};

struct ArrayType : Type {
    AST* elementType;
    AST* size;
    ArrayType(AST *elementType, AST* size, int lin, int col): Type(TYPE_ARRAY, lin, col) {
        this->elementType = elementType;
        this->size = size;
    }
};

struct TemplateType : Type {
    AST* rootType;
    std::vector<AST*> subType;
    TemplateType(AST* rootType, std::vector<AST*> subType, int lin, int col): Type(TYPE_TEMPLATE, lin, col) {
        this->rootType = rootType;
        this->subType = subType;
    }
};

struct NormalType : Type {
    AST* classId;
    NormalType(AST* classId, int lin, int col): Type(TYPE_NORMAL, lin, col) {
        this->classId = classId;
    }
};

struct ForLoop : AST {
    AST* init;
    AST*condition;
    AST* change;
    AST* block;
    ForLoop(AST* init, AST* condition, AST* change, AST* block, int lin, int col): AST(AST_FOR, lin, col) {
        this->init = init;
        this->condition = condition;
        this->change = change;
        this->block = block;
    }
};

struct WhileLoop : AST {
    AST* condition;
    AST* body;
    WhileLoop(AST* condition, AST* body, int lin, int col): AST(AST_WHILE, lin, col) {
        this->condition = condition;
        this->body = body;
    }
};

struct DoWhile : AST {
    AST* condition;
    AST* body;
    DoWhile(AST* condition, AST* body, int lin, int col): AST(AST_DO_WHILE, lin, col) {
        this->condition = condition;
        this->body = body;
    }
};

struct Block : AST {
    std::vector<AST*> codes;
    Block(std::vector<AST*> codes, int lin, int col): AST(AST_BLOCK, lin, col) {
        this->codes = codes;
        this->lin = lin;
        this->col = col;
    }
};

struct ThreeOp : AST {
    AST* condition;
    AST* trueValue;
    AST* falseValue;
    ThreeOp(AST* condition, AST* trueValue, AST* falseValue, int lin, int col): AST(AST_THREE_OP, lin, col) {
        this->condition = condition;
        this->trueValue = trueValue;
        this->falseValue = falseValue;
    }
};

struct Bool : AST {
    std::string bol;
    Bool(std::string bol, int lin, int col): AST(AST_BOOL, lin, col) {
        this->bol = bol;
    }
};

struct Null : AST {
    Null(int lin, int col): AST(AST_NULL, lin, col) {}
};

struct Id : AST {
    std::string name;
    Id(std::string name, int lin, int col): AST(AST_ID, lin, col) {
        this->name = name;
    }
};

struct Goto : AST {
    std::string target;
    Goto(std::string target, int lin, int col) :AST(AST_GOTO, lin, col) {
        this->target = target;
        this->lin = lin;
        this->col = col;
    }
};

struct MemberAccess : AST {
    AST* parent;
    std::string member;
    MemberAccess(AST* parent, std::string member, int lin, int col): AST(AST_MEMBER_ACCESS, lin, col) {
        this->parent = parent;
        this->member = member;
    }
};

struct Call : AST {
    std::vector<AST*> args;
    AST* fnid;
    Call(AST* fnid, std::vector<AST*> args, int lin, int col) : AST(AST_CALL, lin, col) {
        this->fnid = fnid;
        this->args = args;
    }
};

struct ElementGet : AST {
    AST* address;
    AST* position;
    ElementGet(AST* add, AST* pos, int lin, int col): AST(AST_ELEMENT_GET, lin, col) {
        this->address = add;
        this->position = pos;
    }
};

struct BinOpNode : AST {
    AST *left, *right;
    std::string op{};
    BinOpNode(std::string op, AST *left, AST *right, int lin, int col): AST(AST_BIN_OP, lin, col) {
        this->op = std::move(op);
        this->left = left;
        this->right = right;
    }
};

struct Number : AST {
    std::string number;
    explicit Number(std::string number, int lin, int col): AST(AST_DIGIT, lin, col) {
        this->number = std::move(number);
    }
};

struct Char : AST {
    char c;
    explicit Char(std::string c, int lin, int col): AST(AST_CHAR, lin, col) {
        this->c = c[0];
    }
    explicit Char(char c, int lin, int col) : AST(AST_CHAR, lin, col) {
        this->c = c;
    }
};

struct Array : AST {
    std::vector<AST*> elements;
    explicit Array(std::vector<AST*> elements, int lin, int col): AST(AST_ARRAY, lin, col) {
        this->elements = elements;
    }
};

struct Neg : AST {
    AST* value;
    explicit Neg(AST* value, int lin, int col): AST(AST_NEG, lin, col) {
        this->value = value;
    }
};

struct AssignNode : AST {
    std::string op;
    AST* src;
    AST* dst;
    AssignNode(std::string oper, AST* tdst, AST* tsrc, int lin, int col): AST(AST_ASSIGN_NODE, col, lin) {
        if (oper != "=") {
            op = oper.substr(0, oper.find('='));
            src = new BinOpNode(op, tdst, tsrc, lin, col);
            dst = tdst;
        } else {
            src = tsrc;
            dst = tdst;
            op = "=";
        }
    }
};

struct Break : AST {
    Break(int lin, int col) : AST(AST_BREAK, lin, col) {

    }
};

struct Continue : AST {
    Continue(int lin, int col) : AST(AST_CONTINUE, lin, col) {

    }
};

struct Return : AST {
    AST* value;
    Return(AST* value, int lin, int col): AST(AST_RETURN, lin, col) {
        this->value = value;
    }
};

struct SelfChangeNode : AST {
    AST* value;
    bool incOrDec; // true -> inc, false -> dec
    bool isPre; // true -> pre, false -> no pre
    AST* expand;
    SelfChangeNode(AST* value, bool incOrDec, bool isPre, int col, int lin): AST(AST_SELF_CHANGE, col, lin) {
        this->value = value;
        this->incOrDec = incOrDec;
        this->isPre = isPre;
        expand = new AssignNode(incOrDec? "+=":"-=", value, new Number("1", lin, col), lin, col);
    }
};

#endif //MICALANG_AST_H
