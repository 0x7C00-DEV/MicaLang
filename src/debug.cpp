//
// Created by Lenovo on 2026/9/11.
//
#include "../include/debug.h"

inline std::string printIndent(int indent)  {
    std::string res;
    while (indent--) res += "    ";
    return res;
}

void showAST(AST* tree, int indent, const std::string& fo, const std::string& eo)  {
    switch (tree->kind) {
        case AST::AST_BIN_OP: {
            std::cout << printIndent(indent) << fo << "BinOp<'" << ((BinOpNode*)tree)->op << "'> {\n";
            showAST(((BinOpNode*)tree)->left, indent+1, "LEFT: ", ",\n");
            showAST(((BinOpNode*)tree)->right, indent+1, "RIGHT: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_DIGIT: {
            std::cout << printIndent(indent) << fo << "Digit<" << ((Number*)tree)->number << ">" << eo;
            break;
        }
        case AST::AST_CHAR: {
            std::cout << printIndent(indent) << fo << "Char<'" << ((Char*)tree)->c << "'>" << eo;
            break;
        }
        case AST::AST_ARRAY: {
            std::cout << printIndent(indent) << fo << "[\n";
            std::vector<AST*> elements;
            elements = ((Array*)tree)->elements;
            for (int i=0; i<elements.size(); ++i)
                showAST(elements[i], indent+1, std::to_string(i)+": ", "\n");
            std::cout << printIndent(indent) << "]" << eo;
            break;
        }
        case AST::AST_NEG: {
            std::cout << printIndent(indent) << fo << "Neg {\n";
            showAST(((Neg*)tree)->value, indent+1, "NEG: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_ELEMENT_GET: {
            auto id = ((ElementGet*)tree)->address;
            auto pos = ((ElementGet*)tree)->position;
            std::cout << printIndent(indent) << fo << "ElementGet {\n";
            showAST(id, indent+1, "ADDRESS: ", ",\n");
            showAST(pos, indent+1, "POSITIOM: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_CALL: {
            auto id = ((Call*)tree)->fnid;
            auto args = ((Call*)tree)->args;
            std::cout << printIndent(indent) << fo << "Call {\n";
            showAST(id, indent+1, "FUNC: ", ",\n");

            if (!args.empty()) {
                std::cout << printIndent(indent+1) << "ARGS: [\n";
                for (int i=0; i<args.size(); ++i)
                    showAST(args[i], indent+2, std::to_string(i)+": ", ",\n");
                std::cout << printIndent(indent+1) << "]\n";
            }

            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_MEMBER_ACCESS: {
            auto p = ((MemberAccess*)tree)->parent;
            auto m = ((MemberAccess*)tree)->member;
            std::cout << printIndent(indent) << fo << "MemberAccess {\n";
            showAST(p, indent+1, "PARENT: ", ",\n");
            std::cout << printIndent(indent+1) << "MEMBER: " << m << std::endl;
            std::cout << printIndent(indent) << "}" << eo;
             break;
        }
        case AST::AST_ID: {
            std::cout << printIndent(indent) << fo << "Id<" << ((Id*)tree)->name << ">" << eo;
            break;
        }
        case AST::AST_NULL: {
            std::cout << printIndent(indent) << fo << "Null" << eo;
            break;
        }
        case AST::AST_BOOL: {
            std::cout << printIndent(indent) << fo << "Bool<" << ((Bool*)tree)->bol << ">" << eo;
            break;
        }
        case AST::AST_THREE_OP: {
            AST* condition = ((ThreeOp*)tree)->condition;
            AST* tvalue    = ((ThreeOp*)tree)->trueValue;
            AST* fvalue    = ((ThreeOp*)tree)->falseValue;
            std::cout << printIndent(indent) << fo << "ThreeOperatorValue {\n";
            showAST(condition, indent+1, "CONDITION: ", ",\n");
            showAST(tvalue, indent+1, "TRUE: ", ",\n");
            showAST(fvalue, indent+1, "FALSE: ", "\n");
            std::cout << printIndent(indent) << "}" << eo << std::endl;
            break;
        }
    }
}
