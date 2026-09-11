//
// Created by Lenovo on 2026/9/11.
//

#ifndef MICALANG_AST_H
#define MICALANG_AST_H
#include <string>
#include <utility>
#include <vector>

struct AST {
    enum TKind {
        AST_BIN_OP, AST_DIGIT, AST_CHAR, AST_ARRAY, AST_NEG, AST_ELEMENT_GET, AST_CALL,
        AST_MEMBER_ACCESS, AST_ID
    } kind;

    explicit AST(TKind kind) {
        this->kind = kind;
    }
};

struct Id : AST {
    std::string name;
    Id(std::string name): AST(AST_ID) {
        this->name = name;
    }
};

struct MemberAccess : AST {
    AST* parent;
    std::string member;
    MemberAccess(AST* parent, std::string member): AST(AST_MEMBER_ACCESS) {
        this->parent = parent;
        this->member = member;
    }
};

struct Call : AST {
    std::vector<AST*> args;
    AST* fnid;
    Call(AST* fnid, std::vector<AST*> args) : AST(AST_CALL) {
        this->fnid = fnid;
        this->args = args;
    }
};

struct ElementGet : AST {
    AST* address;
    AST* position;
    ElementGet(AST* add, AST* pos): AST(AST_ELEMENT_GET) {
        this->address = add;
        this->position = pos;
    }
};

struct BinOpNode : AST {
    AST *left, *right;
    std::string op{};
    BinOpNode(std::string op, AST *left, AST *right): AST(AST_BIN_OP) {
        this->op = std::move(op);
        this->left = left;
        this->right = right;
    }
};

struct Number : AST {
    std::string number;
    explicit Number(std::string number): AST(AST_DIGIT) {
        this->number = std::move(number);
    }
};

struct Char : AST {
    char c;
    explicit Char(std::string c): AST(AST_CHAR) {
        this->c = c[0];
    }
    explicit Char(char c) : AST(AST_CHAR) {
        this->c = c;
    }
};

struct Array : AST {
    std::vector<AST*> elements;
    explicit Array(std::vector<AST*> elements): AST(AST_ARRAY) {
        this->elements = elements;
    }
};

struct Neg : AST {
    AST* value;
    explicit Neg(AST* value): AST(AST_NEG) {
        this->value = value;
    }
};

#endif //MICALANG_AST_H
