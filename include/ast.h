//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_AST_H
#define MICALANG_AST_H
#include <complex>
#include <string>
#include <utility>
#include "position.h"
#include <vector>

struct AST {
    Position begin;
    Position end;
    enum TKind {
        AST_BIN_OP, AST_DIGIT, AST_CHAR, AST_ARRAY, AST_NEG, AST_ELEMENT_GET, AST_CALL,
        AST_MEMBER_ACCESS, AST_ID, AST_BOOL, AST_NULL, AST_THREE_OP, AST_SELF_CHANGE,
        AST_ASSIGN_NODE, AST_RETURN, AST_CONTINUE, AST_BREAK, AST_GOTO, AST_BLOCK,
        AST_FOR, AST_WHILE, AST_DO_WHILE, AST_SWITCH, AST_TYPE, AST_VAR_DEF, AST_VAR_DEF_GRP,
        AST_CASE, AST_LABEL, AST_IF, AST_FUNC_DEF, AST_INTERFACE, AST_FUNC_TAG, AST_CLASS,
        AST_NEW_CLASS, AST_IMPORT
    } kind;

    explicit AST(TKind kind, Position begin, Position end): begin(std::move(begin)), end(std::move(end)) {
        this->kind = kind;
    }
};

enum AccessType { APUBLIC, APRIVATE, APROTECTED };

struct Import : AST {
    std::string path;
    std::string align;
    Import(std::string path, std::string align, Position begin, Position end): AST(AST_IMPORT, begin, end) {
        this->path = path;
        this->align = align;
    }
};

struct NewClass : AST {
    std::string className;
    std::vector<AST*> initArgs;
    std::vector<AST*> ttypes;
    NewClass(std::string className, std::vector<AST*> initArgs, std::vector<AST*> ttypes, Position begin, Position end) : AST(AST_NEW_CLASS, begin, end) {
        this->className = className;
        this->initArgs = initArgs;
        this->ttypes = ttypes;
    } 
};

struct Class : AST {
    std::string name;
    std::vector<AST*> fields;
    std::vector<std::string> templates;
    std::vector<AST*> methods;
    std::string extend;
    std::vector<std::string> impls;

    Class(std::string name, 
        std::vector<AST*> fields, 
        std::vector<AST*> methods, 
        std::string extend, 
        std::vector<std::string> impls, 
        std::vector<std::string> templates,
         Position begin, Position end)
        :AST(AST_CLASS, std::move(begin), std::move(end)) {
            this->name = name;
            this->fields = fields;
            this->methods = methods;
            this->extend = extend;
            this->impls = impls;
            this->templates = templates;
        }
};

struct Interface : AST {
    std::string name;

    struct FunctionTag : AST {
        std::string name;
        AST* funcType;
        AccessType at;
        FunctionTag(std::string fname, AST* funcType, AccessType at, Position begin, Position end): AST(AST_FUNC_TAG, begin, end) {
            this->name = fname;
            this->funcType = funcType;
            this->at = at;
        }
    };

    std::vector<AST*> funcs;

    Interface(std::string name, std::vector<AST*> funcs, Position begin, Position end): AST(AST_INTERFACE, begin, end) {
        this->name = std::move(name);
        this->funcs = std::move(funcs);
    }
};


struct Type : AST {
    enum TPKind { TYPE_ARRAY, TYPE_TEMPLATE, TYPE_NORMAL, TYPE_FUNC } tpKind;
    Type(TPKind tp_kind, Position begin, Position end) : AST(AST_TYPE, begin, end), tpKind(tp_kind) {}
};

struct FuncType : Type {
    AST* retType;
    std::vector<AST*> args;
    std::vector<std::string> templates;
    FuncType(AST* retType, std::vector<AST*> args, std::vector<std::string> templates, Position begin, Position end): Type(TYPE_FUNC, begin, end) {
        this->retType = retType;
        this->args = args;
        this->templates = templates;
    }
};


struct Func : AST {
    std::string name;
    AST* body;
    std::vector<AST*> args;
    bool isNative;
    AST* ftype;
    AccessType at;
    std::vector<std::string> templates;
    Func(std::string name, AST* body, std::vector<AST*> args, AST* ftype , bool isNative, Position begin, Position end) : AST(AST_FUNC_DEF, begin, end) {
        this->name = name;
        this->body = body;
        this->args = args;
        this->ftype = ftype;
        this->isNative = isNative;
        this->templates = ((FuncType*)ftype)->templates;
        at = APRIVATE;
    }
};

struct If : AST {
    AST* condition;
    AST* tblock;
    AST* fblock;
    If(AST* condition, AST* tblock, AST* fblock, Position begin, Position end): AST(AST_IF, begin, end) {
        this->condition = condition;
        this->tblock = tblock;
        this->fblock = fblock;
    }
};

struct Case : AST {
    AST* value;
    AST* block;
    Case(AST* value, AST* block, Position begin, Position end): AST(AST_CASE, begin, end) {
        this->value = value;
        this->block = block;
    }
};

struct Switch : AST {
    AST* value;
    std::vector<AST*> cases;
    Switch(AST* value, std::vector<AST*> cases, Position begin, Position end): AST(AST_SWITCH, begin, end) {
        this->value = value;
        this->cases = cases;
    }
};

struct Label : AST {
    AST* name;
    Label(AST* name, Position begin, Position end): AST(AST_LABEL, begin, end) {
        this->name = name;
    }
};

struct VarDefGrp : AST {
    std::vector<AST*> vars;
    VarDefGrp(std::vector<AST*> vars, Position begin, Position end): AST(AST_VAR_DEF_GRP, begin, end) {
        this->vars = vars;
    }
};

struct VarDef : AST {
    std::string name;
    AST* type;
    AST* init;
    VarDef(std::string name, AST* type, AST* init, Position begin, Position end): AST(AST_VAR_DEF, begin, end) {
        this->name = name;
        this->type = type;
        this->init = init;
    }
};

struct ArrayType : Type {
    AST* elementType;
    AST* size;
    ArrayType(AST *elementType, AST* size, Position begin, Position end): Type(TYPE_ARRAY, begin, end) {
        this->elementType = elementType;
        this->size = size;
    }
};

struct TemplateType : Type {
    AST* rootType;
    std::vector<AST*> subType;
    TemplateType(AST* rootType, std::vector<AST*> subType, Position begin, Position end): Type(TYPE_TEMPLATE, begin, end) {
        this->rootType = rootType;
        this->subType = subType;
    }
};

struct NormalType : Type {
    AST* classId;
    NormalType(AST* classId, Position begin, Position end): Type(TYPE_NORMAL, begin, end) {
        this->classId = classId;
    }
};

struct ForLoop : AST {
    AST* init;
    AST*condition;
    AST* change;
    AST* block;
    ForLoop(AST* init, AST* condition, AST* change, AST* block, Position begin, Position end): AST(AST_FOR, begin, end) {
        this->init = init;
        this->condition = condition;
        this->change = change;
        this->block = block;
    }
};

struct WhileLoop : AST {
    AST* condition;
    AST* body;
    WhileLoop(AST* condition, AST* body, Position begin, Position end): AST(AST_WHILE, begin, end) {
        this->condition = condition;
        this->body = body;
    }
};

struct DoWhile : AST {
    AST* condition;
    AST* body;
    DoWhile(AST* condition, AST* body, Position begin, Position end): AST(AST_DO_WHILE, begin, end) {
        this->condition = condition;
        this->body = body;
    }
};

struct Block : AST {
    std::vector<AST*> codes;
    Block(std::vector<AST*> codes, Position begin, Position end): AST(AST_BLOCK, begin, end) {
        this->codes = codes;
    }
};

struct ThreeOp : AST {
    AST* condition;
    AST* trueValue;
    AST* falseValue;
    ThreeOp(AST* condition, AST* trueValue, AST* falseValue, Position begin, Position end): AST(AST_THREE_OP, begin, end) {
        this->condition = condition;
        this->trueValue = trueValue;
        this->falseValue = falseValue;
    }
};

struct Bool : AST {
    std::string bol;
    Bool(std::string bol, Position begin, Position end): AST(AST_BOOL, begin, end) {
        this->bol = bol;
    }
};

struct Null : AST {
    Null(Position begin, Position end): AST(AST_NULL, begin, end) {}
};

struct Id : AST {
    std::string name;
    Id(std::string name, Position begin, Position end): AST(AST_ID, begin, end) {
        this->name = name;
    }
};

struct Goto : AST {
    std::string target;
    Goto(std::string target, Position begin, Position end) :AST(AST_GOTO, begin, end) {
        this->target = target;
    }
};

struct MemberAccess : AST {
    AST* parent;
    std::string member;
    std::vector<AST*> templates;
    MemberAccess(AST* parent, std::string member, std::vector<AST*> templates, Position begin, Position end): AST(AST_MEMBER_ACCESS, begin, end) {
        this->parent = parent;
        this->member = member;
        this->templates = templates;
    }
};

struct Call : AST {
    std::vector<AST*> args;
    AST* fnid;
    std::vector<AST*> templates;
    Call(AST* fnid, std::vector<AST*> args, std::vector<AST*> templates, Position begin, Position end) : AST(AST_CALL, begin, end) {
        this->fnid = fnid;
        this->args = args;
        this->templates = templates;
    }
};

struct ElementGet : AST {
    AST* address;
    AST* position;
    ElementGet(AST* add, AST* pos, Position begin, Position end): AST(AST_ELEMENT_GET, begin, end) {
        this->address = add;
        this->position = pos;
    }
};

struct BinOpNode : AST {
    AST *left, *right;
    std::string op{};
    BinOpNode(std::string op, AST *left, AST *right, Position begin, Position end): AST(AST_BIN_OP, begin, end) {
        this->op = std::move(op);
        this->left = left;
        this->right = right;
    }
};

struct Number : AST {
    std::string number;
    explicit Number(std::string number, Position begin, Position end): AST(AST_DIGIT, begin, end) {
        this->number = std::move(number);
    }
};

struct Char : AST {
    char c;
    explicit Char(std::string c, Position begin, Position end): AST(AST_CHAR, begin, end) {
        this->c = c[0];
    }
    explicit Char(char c, Position begin, Position end) : AST(AST_CHAR, begin, end) {
        this->c = c;
    }
};

struct Array : AST {
    std::vector<AST*> elements;
    explicit Array(std::vector<AST*> elements, Position begin, Position end): AST(AST_ARRAY, begin, end) {
        this->elements = elements;
    }
};

struct Neg : AST {
    AST* value;
    explicit Neg(AST* value, Position begin, Position end): AST(AST_NEG, begin, end) {
        this->value = value;
    }
};

struct AssignNode : AST {
    std::string op;
    AST* src;
    AST* dst;
    AssignNode(std::string oper, AST* tdst, AST* tsrc, Position begin, Position end): AST(AST_ASSIGN_NODE, begin, end) {
        if (oper != "=") {
            op = oper.substr(0, oper.find('='));
            src = new BinOpNode(op, tdst, tsrc, begin, end);
            dst = tdst;
        } else {
            src = tsrc;
            dst = tdst;
            op = "=";
        }
    }
};

struct Break : AST {
    Break(Position begin, Position end) : AST(AST_BREAK, std::move(begin), std::move(end)) {

    }
};

struct Continue : AST {
    Continue(Position begin, Position end) : AST(AST_CONTINUE, std::move(begin), std::move(end)) {

    }
};

struct Return : AST {
    AST* value;
    Return(AST* value, Position begin, Position end): AST(AST_RETURN, begin, end) {
        this->value = value;
    }
};

struct SelfChangeNode : AST {
    AST* value;
    bool incOrDec; // true -> inc, false -> dec
    bool isPre; // true -> pre, false -> no pre
    AST* expand;
    SelfChangeNode(AST* value, bool incOrDec, bool isPre, Position begin, Position end): AST(AST_SELF_CHANGE, begin, end) {
        this->value = value;
        this->incOrDec = incOrDec;
        this->isPre = isPre;
        expand = new AssignNode(incOrDec? "+=":"-=", value, new Number("1", begin, end), begin, end);
    }
};

#endif //MICALANG_AST_H
