//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_AST_H
#define MICALANG_AST_H
#include <string>
#include <utility>
#include <vector>

struct AST {
    int lin, col;
    enum TKind {
        AST_BIN_OP, AST_DIGIT, AST_CHAR, AST_ARRAY, AST_NEG, AST_ELEMENT_GET, AST_CALL,
        AST_MEMBER_ACCESS, AST_ID, AST_BOOL, AST_NULL, AST_THREE_OP
    } kind;

    explicit AST(TKind kind, int lin, int col) {
        this->kind = kind;
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

#endif //MICALANG_AST_H
