//
// Created by Lenovo on 2026/9/11.
//
#include "../include/debug.h"
#include "iostream"

inline std::string printIndent(int indent)  {
    std::string res;
    while (indent--) res += "    ";
    return res;
}

void showAST(AST* tree, int indent, const std::string& fo, const std::string& eo)  {
    if (!tree) {
        std::cout << printIndent(indent) << fo << "c_null" << eo;
        return;
    }
    switch (tree->kind) {
        case AST::AST_BIN_OP: {
            std::cout << printIndent(indent) << fo << "BinOp<'" << ((BinOpNode*)tree)->op << "'> {\n";
            showAST(((BinOpNode*)tree)->left, indent+1, "LEFT: ", ",\n");
            showAST(((BinOpNode*)tree)->right, indent+1, "RIGHT: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_FUNC_DEF: {
            auto tmp = (Func*) tree;
            std::cout << printIndent(indent) << fo << "Function {\n";
            std::cout << printIndent(indent+1) << "NAME: " << tmp->name << ",\n";
            showAST(tmp->ftype, indent+1, "FUNC_TYPE: ", ",\n");
            if (!tmp->args.empty()) {
                std::cout << printIndent(indent+1) << "ARGS: [\n";
                for (int i=0; i<tmp->args.size(); ++i)
                    showAST(tmp->args[i], indent+2, std::to_string(i)+": ", ",\n");
                std::cout << printIndent(indent+1) << "],\n";
            }
            if (!tmp->templates.empty()) {
                std::cout << printIndent(indent+1) << "TEMP: [ ";
                for (const auto& i : tmp->templates)
                    std::cout << i << ", ";
                std::cout << printIndent(indent) << "],\n";
            }
            if (!tmp->isNative) showAST(tmp->body, indent+1, "BODY: ", "\n");
            else std::cout << printIndent(indent+1) << "(NativeFunction)\n";
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_IF: {
            auto tmp = (If*) tree;
            std::cout << printIndent(indent) << fo << "If {\n";
            showAST(tmp->condition, indent+1, "COND: ", ",\n");
            showAST(tmp->tblock, indent+1, "TRUE: ", ",\n");
            if (tmp->fblock)
                showAST(tmp->fblock, indent+1, "FALSE: ", "\n");
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
            auto tmp = ((Call*)tree)->templates;
            std::cout << printIndent(indent) << fo << "Call {\n";
            showAST(id, indent+1, "FUNC: ", ",\n");

            if (!tmp.empty()) {
                std::cout << printIndent(indent+1) << "TMP: [ \n";
                for (int i=0; i<tmp.size(); ++i)
                    showAST(tmp[i], indent+2, std::to_string(i)+":", ",\n");
                std::cout << printIndent(indent+1) << "],\n";
            }

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
            auto t = ((MemberAccess*)tree)->templates;
            std::cout << printIndent(indent) << fo << "MemberAccess {\n";
            if (!t.empty()) {
                std::cout << printIndent(indent+1) << "TPM: [\n";
                for (int i=0; i<t.size(); ++i)
                    showAST(t[i], indent+2, std::to_string(i)+": ", ",\n");
                std::cout << printIndent(indent+1)  << "],\n";
            }
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
        case AST::AST_SELF_CHANGE: {
            auto tmp = (SelfChangeNode*) tree;
            std::cout << printIndent(indent) << fo << "SelfChange <" << ((tmp->incOrDec)? "INC":"DEC") << "> {\n";
            showAST(tmp->value, indent+1, "DST: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_ASSIGN_NODE: {
            auto tmp = (AssignNode*) tree;
            std::cout << printIndent(indent) << fo << "Assign <'" << tmp->op << "'> {\n";
            showAST(tmp->dst, indent+1, "DST: ", ",\n");
            showAST(tmp->src, indent+1, "SRC: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_RETURN: {
            std::cout << printIndent(indent) << fo << "Return {\n";
            showAST(((Return*)tree)->value, indent+1, "VALUE: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_CONTINUE: {
            std::cout << printIndent(indent) << fo << "<ContinueStmt>" << eo;
            break;
        }
        case AST::AST_BREAK: {
            std::cout << printIndent(indent) << fo << "<BreakStmt>" << eo;
            break;
        }
        case AST::AST_GOTO: {
            std::cout << printIndent(indent) << fo << "<Goto -> " << (((Goto*)tree)->target) << ">" << eo;
            break;
        }
        case AST::AST_BLOCK: {
            std::cout << printIndent(indent) << fo << "Block {\n";
            for (int i=0; i<((Block*)tree)->codes.size(); ++i)
                showAST(((Block*)tree)->codes[i], indent+1, std::to_string(i)+": ", ",\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_FOR: {
            std::cout << printIndent(indent) << fo << "ForLoop {\n";

            auto tmp = (ForLoop*) tree;
            showAST(tmp->init, indent+1, "INIT: ", ",\n");
            showAST(tmp->condition, indent+1, "COND: ", ",\n");
            showAST(tmp->change, indent+1, "CHANGE: ", ",\n");
            showAST(tmp->block, indent+1, "BODY: ", "\n");

            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_WHILE: {
            std::cout << printIndent(indent) << fo << "WhileLoop {\n";

            showAST(((WhileLoop*)tree)->condition, indent+1, "COND: ", ",\n");
            showAST(((WhileLoop*)tree)->body, indent+1, "BODY: ", "\n");

            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_DO_WHILE: {
            auto tmp = (DoWhile*) tree;

            std::cout << printIndent(indent) << fo << "DoWhile {\n";
            showAST(tmp->condition, indent+1, "COND: ", ",\n");
            showAST(tmp->body, indent+1, "BODY: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_SWITCH: {
            auto tmp = (Switch*) tree;
            std::cout << printIndent(indent) << fo << "Switch {\n";
            showAST(tmp->value, indent+1, "VALUE: ", ",\n");
            for (auto i : tmp->cases)
                showAST(i, indent+1, "", ",\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_TYPE: {
            auto tp = ((Type*)tree)->tpKind;
            std::cout << printIndent(indent) << fo << "Type {\n";
            if (tp == Type::TYPE_ARRAY) {
                std::cout << printIndent(indent+1) << "KIND: ARRAY, \n";
                auto v = (ArrayType*) tree;
                showAST(v->elementType, indent+1, "ELEMENT_TYPE: ", ",\n");
                showAST(v->size, indent+1, "ARRAY_SIZE: ", "\n");
            } else if (tp == Type::TYPE_FUNC) {
                std::cout << printIndent(indent+1) << "KIND: FUNC, \n";
                auto tmp = (FuncType*) tree;
                showAST(tmp->retType, indent+1, "RETURN: ", ",\n");
                if (!tmp->args.empty()) {
                    std::cout << printIndent(indent+1) << "ARGS: [\n";
                    for (int i=0; i<tmp->args.size(); ++i)
                        showAST(tmp->args[i], indent+2, std::to_string(i)+": ", ",\n");
                    std::cout << printIndent(indent+1) << "]\n";
                }
            } else if (tp == Type::TYPE_NORMAL) {
                std::cout << printIndent(indent+1) << "KIND: NORMAL, \n";
                showAST(((NormalType*)tree)->classId, indent+1, "CLASS: ", "\n");
            } else if (tp == Type::TYPE_TEMPLATE) {
                std::cout << printIndent(indent+1) << "KIND: TEMPLATE, \n";
                auto tmp = (TemplateType*) tree;
                showAST(tmp->rootType, indent+1, "ROOT_TYPE: ", ",\n");

                std::cout << printIndent(indent+1) << "Templates: [\n";
                for (int i=0; i<tmp->subType.size(); ++i)
                    showAST(tmp->subType[i], indent+2, std::to_string(i)+": ", ",\n");
                std::cout << printIndent(indent+1) << "]\n";
            }
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_VAR_DEF: {
            std::cout << printIndent(indent) << fo << "Var {\n";
            auto tmp = (VarDef*) tree;
            std::cout << printIndent(indent+1) << "NAME: " << tmp->name << ",\n";
            showAST(tmp->type, indent+1, "TYPE: ", ",\n");
            if (tmp->init)
                showAST(tmp->init, indent+1, "INIT: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_VAR_DEF_GRP: {
            std::cout << printIndent(indent) << fo << "Vars {\n";
            for (auto i: ((VarDefGrp*)tree)->vars)
                showAST(i, indent+1, "", ",\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_CASE: {
            std::cout << printIndent(indent) << fo << "Case {\n";
            if (((Case*)tree)->value)
                showAST(((Case*)tree)->value, indent+1, "VALUE: ", ",\n");
            showAST(((Case*)tree)->value, indent+1, "BODY: ", "\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_LABEL: {
            std::cout << printIndent(indent) << fo << "LABEL: " << ((Id*)((Label*)tree)->name)->name << eo;
            break;
        }
        case AST::AST_INTERFACE: {
            auto tmp = (Interface*) tree;
            std::cout << printIndent(indent) << fo << "Interface<" << tmp->name << "> {\n";
            for (int i=0; i<tmp->funcs.size(); ++i)
                showAST(tmp->funcs[i], indent+1, std::to_string(i)+": ", ",\n");
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_FUNC_TAG: {
            auto tmp = (Interface::FunctionTag*) tree;
            std::cout << printIndent(indent) << fo << "FunctionTag<" << tmp->name << "> {\n";
            showAST(tmp->funcType, indent+1, "TYPE: ", ",\n");
            std::cout << printIndent(indent+1) << "ACCESS_TYPE:" << tmp->at << "\n";
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_CLASS: {
            auto tmp = (Class*) tree;
            std::cout << printIndent(indent) << fo << "Class <" << tmp->name << "> {\n";
            if (!tmp->extend.empty()) std::cout << printIndent(indent+1) << "EXTEND: " << tmp->extend << ",\n";
            if (!tmp->impls.empty()) {
                std::cout << printIndent(indent+1) << "IMPL: [ ";
                for (const auto& i : tmp->impls) std::cout << i << ", ";
                std::cout << printIndent(indent+1) << "],\n";
            }
            if (!tmp->templates.empty()) {
                std::cout << printIndent(indent+1) << "TEMP: [ ";
                for (const auto& i : tmp->templates) std::cout << i << ", ";
                std::cout << printIndent(indent+1) << "],\n";
            }
            if (!tmp->fields.empty()) {
                std::cout << printIndent(indent+1) << "FIELDS: [\n";
                for (auto i : tmp->fields)
                    showAST(i, indent+2, "", ",\n");
                std::cout << printIndent(indent+1) << "],\n";
            }
            if (!tmp->methods.empty()) {
                std::cout << printIndent(indent+1) << "METHODS: [\n";
                for (auto i : tmp->methods)
                    showAST(i, indent+2, "", ",\n");
                std::cout << printIndent(indent+1) << "]\n";
            }
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_NEW_CLASS: {
            auto tmp = (NewClass*) tree;
            std::cout << printIndent(indent) << fo << "NewClass {\n";
            std::cout << printIndent(indent+1) << "NAME: " << tmp->className << ",\n";
            if (!tmp->initArgs.empty()) {
                std::cout << printIndent(indent+1) << "ARGS: [ \n";
                for (auto i : tmp->initArgs)
                    showAST(i, indent+2, "", ",\n");
                std::cout << printIndent(indent+1) << "],\n";
            }
            if (!tmp->ttypes.empty()) {
                std::cout << printIndent(indent+1) << "TEMP: [\n";
                for (int i=0; i<tmp->ttypes.size(); ++i)
                    showAST(tmp->ttypes[i], indent+2, std::to_string(i)+": ", ",\n");
                std::cout << printIndent(indent+1) << "],\n";
            }
            std::cout << printIndent(indent) << "}" << eo;
            break;
        }
        case AST::AST_IMPORT: {
            auto import_ = (Import*) tree;
            std::cout << printIndent(indent) << fo << "Import[" << import_->path << ", " << import_->align << "]" << eo;
            break;
        }
    }
}
