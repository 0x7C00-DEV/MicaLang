//
// Created by Lenovo on 2026/9/25.
//
#include "../include/compiler.h"

AstToMtype::AstToMtype(CompileEnvironment* env) {
    this->env = env;
}


MType* AstToMtype::parseImplementType(AST* a) {
    auto tmp = (ImplementsType*) a;
    std::vector<InterfaceSymbol*> symbols;
    for (auto i : tmp->interfaceNames)
        symbols.push_back((InterfaceSymbol*)findInterface(i));
    return new TImplementsType(symbols);
}

MType* AstToMtype::parseTemplateType(AST* a) {
    auto tmp = (TemplateType*) a;
    auto root = getType(tmp->rootType);
    std::vector<MType*> templ;
    for (auto i : tmp->subType)
        templ.push_back(getType(i));
    return new TTemplateType(root, templ);
}

MType* AstToMtype::getType(AST* a) {
    Type* t = (Type*) a;
    if (t->tpKind == Type::TYPE_ARRAY) return parseArrayType(a);
    if (t->tpKind == Type::TYPE_TEMPLATE) return parseTemplateType(a);
    if (t->tpKind == Type::TYPE_NORMAL) return parseIdNode(a);
    if (t->tpKind == Type::TYPE_FUNC) return parseFuncType(a);
    if (t->tpKind == Type::TYEP_IMPLEMENTS) return parseImplementType(a);
    printf("WARN: unknown type %d\n", t->tpKind);
    exit(-1);
}

MType* AstToMtype::parseIdNode(AST* a) {
    auto tmp = ((NormalType*)a)->classId;
    return findClassMember(tmp);
}

MType* AstToMtype::parseArrayType(AST* a) {
    auto tmp = (ArrayType*) a;
    auto elementType = getType(tmp->elementType);
    return new TArrayType(elementType);
}

MType* AstToMtype::parseFuncType(AST* a) {
    auto tmp = (FuncType*) a;
    std::vector<MType*> argTypes;
    for (auto i : tmp->args) argTypes.push_back(getType(i));
    return new FunctionType(
        getType(tmp->retType),
        argTypes
    );
}

MType *AstToMtype::findClassMember(AST *a) {
    if (a->kind == AST::AST_ID) {
        auto name = ((Id*)a)->name;
        if (name == "int")    return new BaseType(BaseType::MINT);
        if (name == "double") return new BaseType(BaseType::MDOUBLE);
        if (name == "char")   return new BaseType(BaseType::MCHAR);
        if (name == "void")   return new BaseType(BaseType::MVOID);
        if (name == "bool")   return new BaseType(BaseType::MBOOL);
        if (name == "str")    return new TArrayType(new BaseType(BaseType::MCHAR));

        Symbol* sym = env->cs.lookup(name);
        if (!sym) {
            std::cout << "ERROR: '" << name << "' not found\n";
            exit(-1);
        }
        if (sym->kind == Symbol::SYM_INTERFACE)
            return new InterfaceType((InterfaceSymbol*)sym);
        if (sym->kind == Symbol::SYM_CLASS)
            return new ClassType(name, sym);
        std::cout << "ERROR: '" << name << "' is not a type\n";
        exit(-1);
    }
    auto parent = ((MemberAccess*)a)->parent;
    auto member = ((MemberAccess*)a)->member;
    return ((ClassSymbol*)((ClassType*)findClassMember(parent))->sym)->members[member];
}

Symbol* AstToMtype::findInterface(AST* a) {
    if (a->kind == AST::AST_ID) {
        Symbol* sym = env->cs.lookup(((Id*)a)->name);
        if (!sym) {
            std::cout << "ERROR: '" << ((Id*)a)->name << "' not found\n";
            exit(-1);
        }
        if (sym->kind != Symbol::SYM_INTERFACE) {
            std::cout << "ERROR: '" << ((Id*)a)->name
                      << "' is not an interface\n";
            exit(-1);
        }
        return sym;
    }

    if (a->kind != AST::AST_MEMBER_ACCESS) {
        std::cout << "ERROR: unknown type '" << a->kind << "'\n";
        exit(-1);
    }

    auto tmp = findClassMember(((MemberAccess*)a)->parent);
    if (tmp->baseType != MType::BT_CLASS) {
        std::cout << "ERROR: unsuppose type " << tmp->baseType << std::endl;
        exit(-1);
    }

    ClassSymbol* cs = ((ClassSymbol*)(((ClassType*)tmp)->sym));
    std::string memberName = ((MemberAccess*)a)->member;

    auto it = cs->members.find(memberName);
    if (it == cs->members.end()) {
        std::cout << "ERROR: class '" << cs->name
                  << "' has no nested interface '" << memberName << "'\n";
        exit(-1);
    }
    if (it->second->baseType != MType::BT_INTERFACE) {
        std::cout << "ERROR: '" << cs->name << "." << memberName
                  << "' is not an interface\n";
        exit(-1);
    }
    return ((InterfaceType*)it->second)->symbol;
}

ClassSymbol *AstToMtype::findClass(std::string name) {
    auto tmp = (ClassSymbol*)env->cs.lookup(name);
    if (!tmp) std::cout << "WARN: name '" << name << "' not found\n";
    return tmp;
}

ByteCode::ByteCode(int opera) {
    this->opera = opera;
}

ByteCode::ByteCode(std::string label) {
    this->label = label;
    isLabel = true;
}

Instruction::Instruction(std::string index, ByteCode a, ByteCode b, ByteCode c) : a(a), b(b), c(c) {
    this->index = index;
}

int Instruction::toIns() {
    return codingInstr(a.opera, b.opera, c.opera);
}

void CurrentCompileTask::fullBackLabel() {
    std::unordered_map<std::string, int> tmp;
    for (int i = 0; i < ins.size(); ++i)
        tmp[ins[i].index] = i;
    for (int i = 0; i < ins.size(); ++i) {
        if (ins[i].a.isLabel) ins[i].a.opera = tmp[ins[i].a.label];
        if (ins[i].b.isLabel) ins[i].b.opera = tmp[ins[i].b.label];
        if (ins[i].c.isLabel) ins[i].c.opera = tmp[ins[i].c.label];
    }
    for (auto& i : ins)
        i.a.isLabel = i.b.isLabel = i.c.isLabel = false;
}

void CurrentCompileTask::emit(std::string index, ByteCode a, ByteCode b, ByteCode c) {
    ins.push_back(Instruction(index, a, b, c));
}

Function* CurrentCompileTask::getCompileResult() {
    fullBackLabel();
    Function *fn = new Function;
    for (auto i : ins)
        fn->ins.push_back(i.toIns());
    fn->name = funcName;
    fn->constants = localConstPool;
    return fn;
}

int CurrentCompileTask::pushLocalConst(MicaValue value) {
    localConstPool.push_back(value);
    return localConstPool.size()-1;
}

CurrentCompileTask::CurrentCompileTask(Scope* current, std::string name) {
    currentScope = functionScope = current;
    this->funcName = name;
}

Symbol* CurrentCompileTask::lookupLocalVar(std::string name) {
    return currentScope->lookupLocalVar(name);
}

std::string CurrentCompileTask::getLabel() {
    return "L" + std::to_string(++localLabelCnt);
}

Scope* Compiler::createScope(Scope::ScopeKind kind) {
    auto tmp = env->cs.createScope(kind);
    getCurrentTsk()->currentScope = tmp;
    return tmp;
}

void Compiler::leaveScope() {
    auto tmp = env->cs.leaveScope();
    getCurrentTsk()->currentScope = tmp? tmp : getCurrentTsk()->functionScope;
}

Compiler::Compiler(CompileEnvironment* environment) {
    this->env = environment;
    conver = new AstToMtype(this->env);
    opera["+"] = BADD;
    opera["-"] = BSUB;
    opera["/"] = BDIV;
    opera["*"] = BMUL;
    opera["<<"] = BSHL;
    opera[">>"] = BSHR;
    opera["%"] = BMOD;
    opera["^"] = BXOR;
    opera["=="] = BEQ;
    opera["!="] = BNEQ;
    opera[">"] = BBIG;
    opera["<"] = BLESS;
    opera[">="] = BEQORBIG;
    opera["<="] = BEQORLESS;
    opera["&"] = BBAND;
    opera["|"] = BBOR;
    opera["&&"] = BAND;
    opera["||"] = BOR;
    optofn["+"] = "__add__";
    optofn["-"] = "__sub__";
    optofn["/"] = "__div__";
    optofn["*"] = "__mul__";
    optofn["<<"] = "__shl__";
    optofn[">>"] = "__shr__";
    optofn["%"] = "__mod__";
    optofn["^"] = "__xor__";
    optofn["=="] = "__eq__";
    optofn["!="] = "__neq__";
    optofn[">"] = "__big__";
    optofn["<"] = "__less__";
    optofn[">="] = "__eqorbig__";
    optofn["<="] = "__eqorless__";
    optofn["&"] = "__bitand__";
    optofn["|"] = "__bitor__";
    optofn["&&"] = "__and__";
    optofn["||"] = "__or__";
}

void Compiler::createTask(std::string name) {
    auto tmp = new CurrentCompileTask(env->cs.createScope(Scope::SFUNCTION), name);
    env->tasks.push_back(tmp);
}

CurrentCompileTask *Compiler::getCurrentTsk() {
    return env->tasks.empty() ? nullptr : env->tasks.back();
}

MType* Compiler::visitBinOpNode(AST* a, MType* expect) {
    auto tmp = (BinOpNode*) a;
    auto it = opera.find(tmp->op);
    if (it == opera.end()) {
        std::cout << "ERROR: operator '" << tmp->op << "' not suppose\n";
        return nullptr;
    }
    int op = it->second;

    MType* operandExpect;
    if (op == BEQ || op == BNEQ || op == BAND || op == BOR ||
        op == BEQORBIG || op == BEQORLESS || op == BBIG || op == BLESS)
        operandExpect = nullptr;
    else if (op == BSHL || op == BSHR || op == BMOD ||
             op == BBAND || op == BBOR || op == BXOR)
        operandExpect = new BaseType(BaseType::MINT);
    else
        operandExpect = expect;

    MType* left  = visitValue(tmp->left,  operandExpect);
    MType* right = visitValue(tmp->right, operandExpect);
    if (left->baseType != MType::BT_ARRAY && left->baseType != MType::BT_CLASS) {
        emit(getLabel(), BIN_OPER, op, 0);
        auto tl = left->__str__();
        auto tr = right->__str__();
        if (tmp->op == "==" || tmp->op == "!=" || tmp->op == ">=" || tmp->op == "<=" || tmp->op == ">" || tmp->op == "<")
            return new BaseType(BaseType::MBOOL);
        if (tl == "double;" || tr == "double;")
            return new BaseType(BaseType::MDOUBLE);
        return left;
    } else if (left->baseType == MType::BT_CLASS) {
        emit(getLabel(), SWAP_SP, 0, 0);
        emit(getLabel(), DUP, 0, 0);
        auto temp = optofn.find(tmp->op);
        if (temp == optofn.end()) {
            std::cout << "ERROR: operator '" << tmp->op << "' unsuppose\n";
            exit(-1);
        }
        storeString(temp->second);
        emit(getLabel(), MEM_GET, 0, 0);
        emit(getLabel(), SWAP, 0, 2);
        emit(getLabel(), CALL, 2, 0);
        return ((FunctionType*)((ClassSymbol*)((ClassType*)left)->sym)->members[temp->second])->retType;
    }  else if (right->baseType == MType::BT_CLASS) {
        emit(getLabel(), DUP, 0, 0);
        auto temp = optofn.find(tmp->op);
        if (temp == optofn.end()) {
            std::cout << "ERROR: operator '" << tmp->op << "' unsuppose\n";
            exit(-1);
        }
        storeString(temp->second);
        emit(getLabel(), MEM_GET, 0, 0);
        emit(getLabel(), SWAP, 0, 2);
        emit(getLabel(), CALL, 2, 0);
        return ((FunctionType*)((ClassSymbol*)((ClassType*)right)->sym)->members[temp->second])->retType;
    } else {
        std::cout << "ERROR: unknown type " << left->baseType << ", " << right->baseType << "\n";
        exit(-1);
    }
}

Symbol *Compiler::getMemberType(AST* node) {
    if (node->kind == AST::AST_ID) {
        Symbol* sym = env->cs.lookup(((Id*)node)->name);
        if (!sym) {
            std::cout << "ERROR: symbol '" << ((Id*)node)->name << "' not found\n";
            exit(-1);
        }
        return sym;
    }
    if (node->kind == AST::AST_MEMBER_ACCESS) {
        auto tmp = getMemberType(((MemberAccess*)node)->parent);
        auto member = ((MemberAccess*)node)->member;
        return ((ClassSymbol*)((ClassType*)tmp)->sym)->syms[member];
    }
    if (node->kind == AST::AST_ELEMENT_GET) {
        auto tmp = (ElementGet*) node;
        auto parentType = getMemberType(tmp->address);
        return parentType;
    }
    if (node->kind == AST::AST_CALL) {
        auto tmp = (Call*) node;
        return getMemberType(tmp->fnid);
    }
    std::cout << "ERROR: type " << node->kind << " not suppose\n";
    exit(-1);
}

MType* Compiler::visitCallNode(AST* a, MType* expect) {
    auto tmp = (Call*) a;
    MType* tp1 = visitValue(tmp->fnid, expect);
    if (tp1->baseType == MType::BT_FUNC) {
        auto tp = (FunctionType*) tp1;
        int argc = (int)tmp->args.size();
        if (tmp->fnid->kind == AST::AST_MEMBER_ACCESS) {
            auto temp = getMemberType(((MemberAccess*)tmp->fnid)->parent);
            if (temp->kind != Symbol::SYM_CLASS) {
                visitValue(((MemberAccess*)tmp->fnid)->parent, nullptr);
                argc += 1;
            }
        }
        for (auto i : tmp->args)
            visitValue(i, expect);
        emit(getLabel(), CALL, argc, 0);
        return tp->retType;
    } else if (tp1->baseType == MType::BT_CLASS) {
        // [object]
        auto csym = ((ClassSymbol*)((ClassType*) tp1)->sym);
        auto tmp1 = csym->members.find("__call__");
        if (tmp1 == csym->members.end()) {
            std::cout << "Class '" << csym->name << "' has no member '__call__'" << std::endl;
            exit(-1);
        }
        if (tmp1->second->baseType != MType::BT_FUNC) {
            std::cout << "'__call__' not a function\n";
            exit(-1);
        }
        auto ftmp = (FunctionType*)tmp1->second;
        auto retTp = ftmp->retType;
        emit(getLabel(), DUP, 0, 0);
        // [obj, obj]
        storeString("__call__");
        // [obj, obj, name]
        emit(getLabel(), MEM_GET, 0, 0);
        // [obj, func]
        emit(getLabel(), SWAP_SP, 0, 0);
        // [func, obj]
        for (auto i : tmp->args)
            visitValue(i, nullptr);
        emit(getLabel(), CALL, tmp->args.size()+1, 0);
        return retTp;
    } else {
        std::cout << "ERROR: type " << tp1->baseType << " can not call\n";
        exit(-1);
    }
}

MType* Compiler::visitElementGet(AST* a, MType* expect) {
    auto arrayId = (ElementGet*)a;
    auto* tpm = visitValue(arrayId->address, expect);
    // [Object]
    if (tpm->baseType == MType::BT_ARRAY) {
        auto tp = (TArrayType*) tpm;
        MType* posType = visitValue(arrayId->position, expect);
        emit(getLabel(), EL_GET, 0, 0);
        if (((BaseType*)posType)->type != BaseType::MINT)
            std::cout << "Warn: " << posType->__str__() << " not int\n";
        return tp->elementType;
    } else if (tpm->baseType == MType::BT_CLASS) {
        // [obj]
        ClassSymbol* cls = ((ClassSymbol*)((ClassType*) tpm)->sym);
        auto func = cls->members.find("__elementget__");
        if (func == cls->members.end()) {
            std::cout << "ERROR: object '" << cls->name
                      << "' has no '__elementget__' member\n";
            exit(-1);
        }
        if (func->second->baseType != MType::BT_FUNC) {
            std::cout << "ERROR: '__elementget__' is not callable\n";
            exit(-1);
        }
        auto ft = (FunctionType*)func->second;
        MType* retT = ft->retType;

        // [obj]
        emit(getLabel(), DUP, 0, 0);        // [obj, obj]
        storeString("__elementget__");      // [obj, obj, str]
        emit(getLabel(), MEM_GET, 0, 0);    // [obj, method]
        emit(getLabel(), SWAP, 0, 1);       // [method, obj]
        visitValue(arrayId->position, ft->argsType[0]);  // [method, obj, pos]
        emit(getLabel(), CALL, 2, 0);       // [retval]
        return retT;
    } else {
        std::cout << "ERROR: unknown type " << tpm->baseType << std::endl;
        exit(-1);
    }
}

MType* Compiler::visitMemberAccess(AST* a, MType* expect) {
    auto tmp = (MemberAccess*) a;
    auto parent = visitValue(tmp->parent, expect);
    storeString(tmp->member);
    emit(getLabel(), MEM_GET, 0, 0);
    return ((ClassSymbol*)((ClassType*)parent)->sym)->members[tmp->member];
}

void Compiler::visitVarDefineGrp(AST* a, bool isGlobal) {
    auto tmp = (VarDefGrp*) a;
    for (auto i : tmp->vars)
        visitVarDefine(i, isGlobal);
}

Vresult Compiler::visitVarDefine(AST* a, bool isGlobal) {
    Vresult res;
    auto tmp = (VarDef *) a;
    MType* tp = (tmp->type)? conver->getType(tmp->type) : nullptr;

    if (!isGlobal) {
        if (!tp && !tmp->init) {
            std::cout << "ERROR: Can't infer the type info of variable '"
                      << tmp->name << "'\n";
            exit(-1);
        }

        int varId = getCurrentTsk()->addLocalVar(tmp->name, tp,
                                                 tmp->init != nullptr);

        if (tmp->init) {
            MType* initType = visitValue(tmp->init, tp);

            if (!tp) {
                tp = initType;
                auto sym = getCurrentTsk()->currentScope
                              ->lookupLocalVar(tmp->name);
                if (sym && sym->kind == Symbol::SYM_VAR)
                    ((VarSymbol*)sym)->type = tp;
            } else if (initType->__str__() != tp->__str__()) {
                std::cout << "WARN: type " << initType->__str__()
                          << ", " << tp->__str__() << std::endl;
            }
            emit(getLabel(), STORE_SVAR, varId, 0);
        }
        res.id = varId;
        res.name = tmp->name;
        res.type = tp;
        return res;
    }

    int varId = addGlobalVar(tmp->name, tp, tmp->init != nullptr);
    if (!tp && !tmp->init) {
        std::cout << "ERROR: Can't infer the type info of variable '"
                  << tmp->name << "'\n";
        exit(-1);
    }
    if (tmp->init) {
        MType* initType = visitValue(tmp->init, tp);
        if (!tp) {
            tp = initType;
            auto sym = env->cs.getGlobalScope()->symbols[tmp->name];
            if (sym && sym->kind == Symbol::SYM_VAR)
                ((VarSymbol*)sym)->type = tp;
        } else if (initType->__str__() != tp->__str__()) {
            std::cout << "WARN: type " << initType->__str__()
                      << ", " << tp->__str__() << std::endl;
        }
        emit(getLabel(), STORE_GVAR, varId, 0);
    }
    res.id = varId;
    res.name = tmp->name;
    res.type = tp;
    return res;
}


MType* Compiler::visitModuleImport(AST* a) {
    auto tmp = (Import*) a;
    std::string path = tmp->path;
    std::string rename = tmp->align;
    ProgramLoader loader(path);
    ModuleSymbol* sym = loader.getModuleTag(path);
    env->cs.registSymbolGbl(rename, sym);
    return new ModuleType(rename, path, sym);
}

Function* Compiler::makeFunction(AST* a) {
    auto tmp = (Func*) a;
    visitFunction(a, false);
    if ( !tmp->isNative && (getCurrentTsk()->ins.empty() || getCurrentTsk()->ins.back().a.opera != RET)) {
        emit(getLabel(), LOAD_NULL, 0, 0);
        emit(getLabel(), RET, 0, 0);
    }
    auto temp = getCurrentTsk()->getCompileResult();
    temp->isNative = tmp->isNative;
    env->tasks.pop_back();
    env->cs.leaveScope();
    return temp;
}

MType* Compiler::visitFunction(AST* a, bool autoend) {
    auto tmp = (Func*) a;
    createTask(tmp->name);
    auto ftp = (FunctionType*)conver->getType(tmp->ftype);
    getCurrentTsk()->fnRetTp = ftp->retType;

    if (autoend && !env->cs.lookup(tmp->name))
        env->cs.registSymbolGbl(tmp->name,
                                new FunctionSymbol(tmp->name, ftp, false));

    if (!tmp->isNative) {
        for (int i = 0; i < tmp->args.size(); ++i)
            visitVarDefine(tmp->args[i], false);
        visitBlock(tmp->body, "", "");
    }
    auto t = ftp;
    if (autoend) endTask(tmp->isNative);
    return t;
}

MType* Compiler::visitValue(AST* a, MType* expect) {
    if (a->kind == AST::AST_ARRAY)
        return visitArray(a, expect);
    if (a->kind == AST::AST_BIN_OP)
        return visitBinOpNode(a, expect);
    if (a->kind == AST::AST_MEMBER_ACCESS)
        return visitMemberAccess(a, expect);
    if (a->kind == AST::AST_ELEMENT_GET)
        return visitElementGet(a, expect);
    if (a->kind == AST::AST_ASSIGN_NODE)
        return visitAssign(a);
    if (a->kind == AST::AST_BOOL)
        return visitBool(a);
    if (a->kind == AST::AST_NULL) {
        emit(getLabel(), LOAD_NULL, 0, 0);
        return new BaseType(BaseType::MNULL);
    }
    if (a->kind == AST::AST_ID)
        return visitId(a);
    if (a->kind == AST::AST_CALL)
        return visitCallNode(a, expect);
    if (a->kind == AST::AST_DIGIT)
        return visitNumber(a);
    if (a->kind == AST::AST_THREE_OP)
        return visitTernOp(a, expect);
    if (a->kind == AST::AST_CHAR)
        return visitChar(a);
    if (a->kind == AST::AST_NEW_CLASS)
        return visitNewClass(a, expect);
    if (a->kind == AST::AST_NEG)         return visitNeg(a, expect);
    if (a->kind == AST::AST_BIT_NOT)     return visitBitNot(a, expect);
    if (a->kind == AST::AST_NOT)         return visitNot(a, expect);
    if (a->kind == AST::AST_SELF_CHANGE) return visitSelfChange(a);
    std::cout << "ERROR: visitValue, kind "<< a->kind << "' not suppose\n";
    return nullptr;
}

MType* Compiler::visitAssign(AST* a) {
    auto tmp = (AssignNode*)a;
    auto dst = tmp->dst;
    auto src = tmp->src;
    if (dst->kind == AST::AST_ID) {
        auto tmpx = (Id*) dst;
        Symbol* s = env->cs.lookup(tmpx->name);
        if (!s) {
            std::cout << "ERROR: in visitAssign name '" << tmpx->name << "' not found\n";
            exit(-1);
        }
        if (s->kind != Symbol::SYM_VAR) {
            std::cout << "ERROR: name '" << tmpx->name << "' not a var\n";
            exit(-1);
        }
        auto temp = (VarSymbol*) s;
        auto vt = visitValue(src, temp->type)->__str__();
        auto st = temp->type->__str__();
        if (st != vt)
            std::cout << "WARN: var '" << tmpx->name << "' type is '" << st << "' but value type is '" << vt << "'\n";
        int oper = (temp->vkind == VarSymbol::Global)? STORE_GVAR : STORE_SVAR;
        emit(getLabel(), oper, temp->id, 0);
        return temp->type;
    }
    if (dst->kind == AST::AST_ELEMENT_GET) {
        auto temp = (ElementGet*) dst;
        MType* obj = visitValue(temp->address, nullptr);
        // [obj]
        if (obj->baseType == MType::BT_ARRAY) {
            MType* valt = visitValue(src, ((TArrayType*)obj)->elementType);
            MType* pos_ = visitValue(temp->position, new BaseType(BaseType::MINT));
            emit(getLabel(), EL_SET, 0, 0);
            if (pos_->__str__() != "int;") std::cout << "WARN: want a int, get '" << pos_->__str__() << "'\n";
            if (obj->baseType != MType::BT_ARRAY) std::cout << "WARN: not a array\n";
            if (valt->__str__() != ((TArrayType*)obj)->elementType->__str__())
                std::cout << "WARN: element type is '" << ((TArrayType*)obj)->elementType->__str__() << "' value type is '" << valt->__str__() << "'\n";
            return valt;
        } else if (obj->baseType == MType::BT_CLASS) {
            // [obj]
            ClassSymbol* cls = ((ClassSymbol*)((ClassType*) obj)->sym);
            auto it = cls->members.find("__elementset__");
            if (it == cls->members.end()) {
                std::cout << "ERROR: object '" << cls->name
                          << "' has no '__elementset__' member\n";
                exit(-1);
            }
            if (it->second->baseType != MType::BT_FUNC) {
                std::cout << "ERROR: '__elementset__' is not callable\n";
                exit(-1);
            }
            auto ft = (FunctionType*)it->second;

            // [obj]
            emit(getLabel(), DUP, 0, 0);        // [obj, obj]
            storeString("__elementset__");      // [obj, obj, str]
            emit(getLabel(), MEM_GET, 0, 0);    // [obj, method]
            emit(getLabel(), SWAP_SP, 0, 0);    // [method, obj]
            visitValue(temp->position, ft->argsType[1]);  // [method, obj, pos]
            visitValue(src,            ft->argsType[2]);  // [method, obj, pos, val]
            emit(getLabel(), CALL, 3, 0);       // ← CALL 3
            emit(getLabel(), POP, 0, 0);
            return ft->retType;
        } else {
            std::cout << "ERROR: error unsuppose type " << obj->baseType << "\n";
            exit(-1);
        }
    }
    if (dst->kind == AST::AST_MEMBER_ACCESS) {
        auto tmpx = (MemberAccess*) dst;
        MType* parentType = visitValue(tmpx->parent, nullptr);
        if (parentType->baseType != MType::BT_CLASS) {
            std::cout << "ERROR: not a class\n"; exit(-1);
        }
        MType* memberType = ((ClassSymbol*)((ClassType*)parentType)->sym)->members[tmpx->member];
        MType* valueType = visitValue(src, memberType);
        storeString(tmpx->member);
        emit(getLabel(), MEM_SET, 0, 0);
        return valueType;
    }
    std::cout << "ERROR: unsuppose type '" << dst->kind << "'\n";
    exit(-1);
}

MType* Compiler::visitStmt(AST* a, std::string begin, std::string end) {
    if (a->kind == AST::AST_FUNC_DEF)
        return visitFunction(a, true);
    if (a->kind == AST::AST_VAR_DEF_GRP) {
        visitVarDefineGrp(a, false);
        return nullptr;
    }
    if (a->kind == AST::AST_IMPORT)
        return visitModuleImport(a);
    if (a->kind == AST::AST_VAR_DEF) {
        visitVarDefine(a, false);
        return nullptr;
    }
    if (a->kind == AST::AST_CONTINUE) {
        visitContinue(a, begin, end);
        return nullptr;
    }
    if (a->kind == AST::AST_BREAK) {
        visitBreak(a, begin, end);
        return nullptr;
    }
    if (a->kind == AST::AST_IF)
        return visitIf(a, begin, end);
    if (a->kind == AST::AST_FOR)
        return visitForLoop(a);
    if (a->kind == AST::AST_DO_WHILE)
        return visitDoWhile(a);
    if (a->kind == AST::AST_WHILE)
        return visitWhileLoop(a);
    if (a->kind == AST::AST_SWITCH)
        return visitSwitch(a);
    if (a->kind == AST::AST_RETURN)
        return visitReturn(a);
    if (a->kind == AST::AST_CLASS)
        return visitClass(a);
    if (a->kind == AST::AST_INTERFACE)
        return visitInterface(a);
    if (a->kind == AST::AST_FUNC_TAG)
        return visitFuncTag(a);
    MType* t = visitValue(a, nullptr);
    if (a->kind != AST::AST_ASSIGN_NODE)
        emit(getLabel(), POP, 0, 0);
    return t;
}

MType* Compiler::visitTernOp(AST* a, MType* expect) {
    std::string false_ = getLabel();
    std::string end = getLabel();
    MType* retTypet,* retTypef;
    auto tmp = (ThreeOp*)a;
    auto tp = visitValue(tmp->condition, expect);
    emit(getLabel(), JMPF, false_, 0);
    retTypet = visitValue(tmp->trueValue, expect);
    emit(getLabel(), JMP, end, 0);
    emit(false_, NOP, 0, 0);
    retTypef = visitValue(tmp->falseValue, expect);
    emit(end, NOP, 0, 0);
    if (retTypef->__str__() != retTypet->__str__())
        std::cout << "WARN: " << retTypet->__str__() << ", " << retTypef->__str__() << std::endl;
    return retTypet;
}

MType* Compiler::visitForLoop(AST* a) {
    createScope(Scope::SNORMAL_BLOCK);
    auto tmp = (ForLoop*)a;
    std::string begin = getLabel();
    std::string changeLabel = getLabel();
    std::string end = getLabel();

    if (tmp->init && tmp->init->kind == AST::AST_BLOCK) {
        for (auto stmt : ((Block*)tmp->init)->codes)
            visitStmt(stmt, "", "");
    }

    emit(begin, NOP, 0, 0);
    MType* tp = nullptr;
    if (tmp->condition) {
        tp = visitValue(tmp->condition, new BaseType(BaseType::MBOOL));
        emit("", JMPF, end, 0);
    }

    visitBlock(tmp->block, changeLabel, end);
    emit(changeLabel, NOP, 0, 0);

    if (tmp->change && tmp->change->kind == AST::AST_BLOCK) {
        for (auto stmt : ((Block*)tmp->change)->codes)
            visitStmt(stmt, "", "");
    }
    emit("", JMP, begin, 0);
    emit(end, NOP, 0, 0);

    if (tp && tp->__str__() != "bool;") {
        std::cout << "WARN: want a bool, but get a '" << tp->__str__() << "'\n";
    }
    leaveScope();
    return nullptr;
}

MType* Compiler::visitWhileLoop(AST* a) {
    createScope(Scope::SNORMAL_BLOCK);
    auto tmp = (WhileLoop*) a;
    std::string begin = getLabel();
    std::string end = getLabel();
    emit(begin, NOP, 0, 0);
    auto tp = visitValue(tmp->condition, new BaseType(BaseType::MBOOL));
    emit(getLabel(), JMPF, end, 0);
    visitBlock(tmp->body, begin, end);
    emit(getLabel(), JMP, begin, 0);
    emit(end, NOP, 0,0);
    if (tp->__str__() != "bool;")
        std::cout << "warn in visitWhile, want a bool value, but get a '" << tp->__str__() << "'\n";
    leaveScope();
    return nullptr;
}

MType* Compiler::visitDoWhile(AST* a) {
    createScope(Scope::SNORMAL_BLOCK);
    auto tmp = (DoWhile*) a;
    std::string begin = getLabel();
    std::string cond  = getLabel();
    std::string end   = getLabel();
    emit(begin, NOP, 0, 0);
    visitBlock(tmp->body, cond, end);
    emit(cond, NOP, 0, 0);
    auto tp = visitValue(tmp->condition, new BaseType(BaseType::MBOOL));
    emit(getLabel(), JMPT, begin, 0);
    emit(end, NOP, 0, 0);
    leaveScope();
    return tp;
}

MType* Compiler::visitSwitch(AST* a) {
    createScope(Scope::SNORMAL_BLOCK);
    auto tmp = (Switch*)a;
    std::string begin = getLabel();
    std::string end = getLabel();
    auto tp = visitValue(tmp->value, nullptr);
    std::string t = tp->__str__();
    std::vector<std::string> types;
    emit(begin, NOP, 0, 0);
    for (auto i: tmp->cases) {
        auto su = (Case*) i;
        if (su->value) {
            createScope(Scope::SNORMAL_BLOCK);
            std::string end_ = getLabel();
            emit(getLabel(), DUP, 0, 0);
            types.push_back(visitValue(su->value, tp)->__str__());
            emit(getLabel(), BIN_OPER, BEQ, 0);
            emit(getLabel(), JMPF, end_, 0);
            visitBlock(su->block, begin, end);
            emit(end_, NOP, 0, 0);
            leaveScope();
        } else {
            visitBlock(su->block, begin, end);
        }
    }
    emit(end, POP, 0, 0);
    for (auto i: types)
        if (i != t) std::cout << "WARN: type '" << t << "' != '" << i << "'\n";
    leaveScope();
    return nullptr;
}

MType* Compiler::visitInterface(AST* a) {
    Interface* i = (Interface*)a;
    auto sym = (InterfaceSymbol*)env->cs.lookup(i->name);
    if (!sym || sym->kind != Symbol::SYM_INTERFACE) {
        std::cout << "ERROR: interface '" << i->name << "' not registered\n";
        exit(-1);
    }

    for (auto j : i->funcs) {
        auto temp = (Interface::FunctionTag*) j;
        sym->labels[temp->name] = new FunctionSymbol(
            temp->name,
            (FunctionType*)conver->getType(temp->funcType),
            false
        );
    }
    return nullptr;
}

MType* Compiler::visitClass(AST* a) {
    auto cls = (Class*) a;
    auto tmp = new ObjClass(cls->name);

    ClassSymbol* super = getClassInfo(cls->extend);
    std::vector<InterfaceSymbol*> impls;
    for (auto i : cls->impls)
        impls.push_back(getInterface(i));

    auto sym = (ClassSymbol*)env->cs.lookup(cls->name);
    if (!sym) {
        int clsId = env->classCnt++;
        sym = new ClassSymbol(cls->name, super, impls,
                              env->moduleName, clsId, {}, {});
        env->cs.registSymbolGbl(cls->name, sym);
    } else {
        sym->super     = super;
        sym->impl      = impls;
        sym->clsModule = env->moduleName;
    }

    tmp->super = getClassObject(cls->extend);
    pushConstG(MicaValue::Object(tmp));

    std::vector<std::string> fields;
    for (auto i : cls->fields) {
        if (i->kind == AST::AST_VAR_DEF) {
            auto vd = (VarDef*)i;
            fields.push_back(vd->name);
            sym->members[vd->name] = conver->getType(vd->type);
            sym->syms[vd->name] = new VarSymbol(vd->name, sym->members[vd->name], false, VarSymbol::ClassMember);
        } else if (i->kind == AST::AST_VAR_DEF_GRP) {
            for (auto j : ((VarDefGrp*)i)->vars) {
                auto vd = (VarDef*)j;
                fields.push_back(vd->name);
                sym->members[vd->name] = conver->getType(vd->type);
                sym->syms[vd->name] = new VarSymbol(vd->name, sym->members[vd->name], false, VarSymbol::ClassMember);
            }
        }
    }
    tmp->fields = fields;
    for (auto i : cls->methods) {
        if (i->kind == AST::AST_FUNC_DEF) {
            auto fn = (Func*)i;
            sym->members[fn->name] = conver->getType(fn->ftype);
            sym->syms[fn->name] = new FunctionSymbol(fn->name, (FunctionType*)sym->members[fn->name], true);
        }
    }

    std::unordered_map<std::string, MicaValue> funcs;
    for (auto i : cls->methods) {
        if (i->kind == AST::AST_FUNC_DEF) {
            auto fn = (Func*)i;
            funcs[fn->name] = MicaValue::Object(makeFunction(fn));
        }
    }

    tmp->methods = funcs;
    sym->checkIsImplement();
    return nullptr;
}

MType* Compiler::visitNewClass(AST* a, MType* expect) {
    auto tmp = (NewClass*) a;
    int pos = -1;
    for (int i = 0; i < (int)env->globalConstPool.size(); ++i) {
        auto k = env->globalConstPool[i];
        if (k.kind == MicaValue::OBJ && k.obj->tp == Obj::USER_DEFING_CLASS &&
            ((ObjClass*)k.obj)->name == tmp->className) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        std::cout << "ERROR: class '" << tmp->className << "' not found \n";
        exit(-1);
    }
    emit(getLabel(), LOAD_GCST, pos, 0);   // [class]
    emit(getLabel(), NEW, 0, 0);           // [obj]
    if (tmp->isCallInit) {
        emit(getLabel(), DUP, 0, 0);       // [obj, obj]
        storeString("__init__");           // [obj, obj, strArr]
        emit(getLabel(), MEM_GET, 0, 0);   // [obj, fn]
        emit(getLabel(), DUP, 1, 0);       // [obj, fn, obj]
        for (auto i : tmp->initArgs)
            visitValue(i, nullptr);        // [obj, fn, obj, args...]
        emit(getLabel(), CALL, (int)tmp->initArgs.size() + 1, 0);  // [obj]
        emit(getLabel(), POP, 0, 0);
    }
    return new ClassType(tmp->className, env->cs.lookup(tmp->className));
}

MType* Compiler::visitArray(AST* a, MType* type) {
    auto tmp = (Array*)a;
    int n = (int)tmp->elements.size();
    loadConstS(pushConstL(MicaValue::Int(n)));
    emit(getLabel(), NEW_ARR, 0, 0);
    MType* elemType = nullptr;
    if (type) {
        if (type->baseType != MType::BT_ARRAY) {
            std::cout << "ERROR: not a array\n";
            exit(-1);
        }
        elemType = ((TArrayType*)type)->elementType;
    }
    for (int i = 0; i < n; ++i) {
        emit(getLabel(), DUP, 0, 0);
        MType* it = visitValue(tmp->elements[i], elemType);
        loadConstS(pushConstL(MicaValue::Int(i)));
        if (!elemType) elemType = it;
        else if (elemType->__str__() != it->__str__()) std::cout << "WARN: element type mismatch: " << it->__str__() << " vs " << elemType->__str__() << "\n";
        emit(getLabel(), EL_SET, 0, 0);
    }
    return new TArrayType(elemType, n);
}

MType* Compiler::visitNumber(AST* a) {
    auto tmp = ((Number*)a)->number;
    if (tmp.find(".") != std::string::npos) {
        loadConstS(pushConstL(MicaValue::Float(std::stof(tmp))));
        return new BaseType(BaseType::MDOUBLE);
    }
    loadConstS(pushConstL(MicaValue::Int(stol(tmp))));
    return new BaseType(BaseType::MINT);
}

MType* Compiler::visitChar(AST* a) {
    loadConstS(pushConstL(MicaValue::Char(((Char*)a)->c)));
    return new BaseType(BaseType::MCHAR);
}

MType* Compiler::visitBool(AST* a) {
    loadConstS(pushConstL(MicaValue::Bool(((Bool*)a)->bol == "true")));
    return new BaseType(BaseType::MBOOL);
}

void Compiler::emit(std::string a, ByteCode b, ByteCode c, ByteCode d) {
    getCurrentTsk()->emit(a, b, c, d);
}

int Compiler::pushConstL(MicaValue v) {
    return getCurrentTsk()->pushLocalConst(v);
}

int Compiler::pushConstG(MicaValue v) {
    env->globalConstPool.push_back(v);
    return  env->globalConstPool.size()-1;
}

void Compiler::loadConstS(int addr) {
    emit(getLabel(), LOAD_SCST, addr, 0);
}

void Compiler::loadConstG(int addr) {
    emit(getLabel(), LOAD_GCST, addr, 0);
}

std::string Compiler::getLabel() {
    return getCurrentTsk()->getLabel();
}

void Compiler::endTask(bool isNative) {
    if (!isNative && (getCurrentTsk()->ins.empty() ||
                      getCurrentTsk()->ins.back().a.opera != RET)) {
        emit(getLabel(), LOAD_NULL, 0, 0);
        emit(getLabel(), RET, 0, 0);
    }

    auto compiled = getCurrentTsk()->getCompileResult();
    compiled->isNative = isNative;
    std::string name = getCurrentTsk()->funcName;

    auto sym = env->cs.lookup(name);
    if (sym && sym->kind == Symbol::SYM_FUNC) {
        auto fsym = (FunctionSymbol*)sym;

        if (fsym->constPoolIdx < 0) {
            fsym->constPoolIdx = env->addFunctionValue(compiled);
        } else {
            auto& slot = env->globalConstPool[fsym->constPoolIdx];
            if (slot.kind == MicaValue::OBJ && slot.obj->tp == Obj::FUNCTION) {
                auto f = (Function*)slot.obj;
                f->ins       = compiled->ins;
                f->constants = compiled->constants;
                f->isNative  = isNative;
            } else {
                env->globalConstPool[fsym->constPoolIdx] =
                        MicaValue::Object(compiled);
            }
        }
    } else {
        env->addFunctionValue(compiled);
    }

    env->tasks.pop_back();
    env->cs.leaveScope();
}

MType *Compiler::visitReturn(AST *a) {
    auto tmp = (Return*)a;
    MType* tp = new BaseType(BaseType::MVOID);
    if (tmp->value) {
        tp = visitValue(tmp->value, getCurrentTsk()->fnRetTp);
    } else {
        emit(getLabel(), LOAD_NULL, 0, 0);
    }
    emit(getLabel(), RET, 0, 0);
    return tp;
}

MType *Compiler::visitFuncTag(AST *) {
    return nullptr;
}

MType *Compiler::visitId(AST *a) {
    auto name = ((Id*)a)->name;
    auto info = getCurrentTsk()->currentScope->lookupSymbol(name);
    if (!info) {
        std::cout << "ERROR: id '" << name << "' is not found.\n";
        exit(-1);
    }

    if (info->kind == Symbol::SYM_VAR) {
        int oper = (((VarSymbol*)info)->vkind == VarSymbol::Global)
                   ? LOAD_GVAR : LOAD_SVAR;
        emit(getLabel(), oper, ((VarSymbol*)info)->id, 0);
        return ((VarSymbol*)info)->type;
    }

    if (info->kind == Symbol::SYM_FUNC) {
        auto fsym = (FunctionSymbol*)info;
        if (fsym->constPoolIdx < 0) {
            auto placeholder = new Function;
            placeholder->name = name;
            fsym->constPoolIdx = env->addFunctionValue(placeholder);
        }

        emit(getLabel(), LOAD_GCST, fsym->constPoolIdx, 0);
        return fsym->type;
    }

    if (info->kind == Symbol::SYM_MODULE) {
        std::cout << "ERROR in visitId, member not suppose\n";
        exit(-1);
    }

    if (info->kind == Symbol::SYM_CLASS) {
        for (int i = 0; i < (int)env->globalConstPool.size(); ++i) {
            auto& k = env->globalConstPool[i];
            if (k.kind == MicaValue::OBJ &&
                k.obj->tp == Obj::USER_DEFING_CLASS &&
                ((ObjClass*)k.obj)->name == name) {
                emit(getLabel(), LOAD_GCST, i, 0);
                return new ClassType(name, info);
            }
        }
        std::cout << "ERROR: class '" << name
                  << "' not found in globalConstPool\n";
        exit(-1);
    }

    std::cout << "ERROR: kind '" << info->kind << "' not suppose\n";
    return nullptr;
}

MType *Compiler::visitNeg(AST *a, MType* expect) {
    auto tmp = (Neg*)a;
    auto val = visitValue(tmp->value, expect);
    emit(getLabel(), BNEG, 0 , 0);
    return val;
}

MType *Compiler::visitSelfChange(AST *a) {
    auto tmp = (SelfChangeNode* )a;
    auto exp = (AssignNode*)tmp->expand;
    if (tmp->isPre) {
        visitAssign(exp);
        return visitValue(exp->dst, nullptr);
    }
    auto temp = visitValue(exp->dst, nullptr);
    visitAssign(exp);
    return temp;
}
MType *Compiler::visitBitNot(AST *pAst, MType* expect) {
    auto bn = (BitNot*) pAst;
    auto tmp = visitValue(bn->value, expect);
    emit(getLabel(), BBNOT, 0, 0);
    return tmp;
}

MType *Compiler::visitNot(AST *pAst, MType* expect) {
    auto bn = (Not*) pAst;
    auto tmp = visitValue(bn->value, expect);
    emit(getLabel(), BNOT, 0, 0);
    return new BaseType(BaseType::MBOOL);
}

MType *Compiler::visitIf(AST *a, std::string begin, std::string end) {
    createScope(Scope::SNORMAL_BLOCK);
    auto tmp = (If*)a;
    std::string if_ = getLabel();
    std::string ie = getLabel();
    auto tp = visitValue(tmp->condition, new BaseType(BaseType::MBOOL));
    if (tp->__str__() != "bool;")
        std::cout << "warn in visitIf, want a bool value, but get a '" << tp->__str__() << "'\n";
    emit(getLabel(), JMPF, if_, 0);
    visitBlock(tmp->tblock, begin, end);
    emit(getLabel(), JMP, ie, 0);
    emit(if_, NOP, 0, 0);
    if (tmp->fblock) visitBlock(tmp->fblock, begin, end);
    emit(ie, NOP, 0, 0);
    leaveScope();
    return nullptr;
}

MType *Compiler::visitBlock(AST *a, std::string begin, std::string end) {
    createScope(Scope::SNORMAL_BLOCK);
    for (auto i:((Block*)a)->codes)
        visitStmt(i, begin, end);
    leaveScope();
    return nullptr;
}

void Compiler::storeArray(std::vector<AST*> array) {
    loadConstS(pushConstL(MicaValue::Int(array.size())));
    emit(getLabel(), NEW_ARR, 0, 0);
    for (int i=0; i<array.size(); ++i) {
        emit(getLabel(), DUP, 0, 0);
        visitValue(array[i], nullptr);
        emit(getLabel(), IMM, i, 0);
        emit(getLabel(), EL_SET, 0, 0);
    }
}

void Compiler::storeString(std::string str) {
    std::vector<AST*> carray;
    for (auto i: str)
        carray.push_back(new Char(i, {}, {}));
    storeArray(carray);
}

void Compiler::visitContinue(AST *a, std::string begin, std::string end) {
    emit(getLabel(), JMP, begin, 0);
}

void Compiler::visitBreak(AST *a, std::string begin, std::string end) {
    emit(getLabel(), JMP, end, 0);
}

int Compiler::getClassId(std::string name) {
    for (int i = 0; i < env->globalConstPool.size(); ++i) {
        auto tmp = env->globalConstPool[i];
        if (tmp.kind == MicaValue::OBJ && tmp.obj->tp == Obj::USER_DEFING_CLASS && ((ObjClass*)tmp.obj)->name == name)
            return i;
    }
    return -1;
}

ObjClass *Compiler::getClassObject(std::string name) {
    for (int i = 0; i < env->globalConstPool.size(); ++i) {
        auto tmp = env->globalConstPool[i];
        if (tmp.kind == MicaValue::OBJ && tmp.obj->tp == Obj::USER_DEFING_CLASS && ((ObjClass*)tmp.obj)->name == name)
            return (ObjClass*)tmp.obj;
    }
    return nullptr;
}

ClassSymbol *Compiler::getClassInfo(std::string name) {
    if (name.empty()) return nullptr;
    auto cls = env->cs.lookup(name);
    if (!cls) return nullptr;
    if (cls->kind != Symbol::SYM_CLASS) return nullptr;
    if (((ClassSymbol*)cls)->name != name) return nullptr;
    return (ClassSymbol*)cls;
}

InterfaceSymbol *Compiler::getInterface(std::string name) {
    if (name.empty()) return nullptr;
    auto cls = env->cs.lookup(name);
    if (!cls) return nullptr;
    if (cls->kind != Symbol::SYM_INTERFACE) return nullptr;
    if (((InterfaceSymbol*)cls)->name != name) return nullptr;
    return (InterfaceSymbol*)cls;
}

int Compiler::getInterfaceId(std::string name) {
    auto tmp = getInterface(name);
    if (!tmp) return -1;
    return tmp->interfaceId;
}

Module* Compiler::getProgram(std::string text, std::string fileName) {
    std::vector<Register> tmp = parser.parseCode(text, fileName);
    for (auto i : tmp)
        if (!i.isSuc) std::cout << i.error << std::endl;

    std::vector<AST*> Class_, globalVar, Fn, Itf, Import;
    for (auto i: tmp) {
        if (i.result->kind == AST::AST_CLASS)
            Class_.push_back(i.result);
        else if (i.result->kind == AST::AST_VAR_DEF ||
                 i.result->kind == AST::AST_VAR_DEF_GRP)
            globalVar.push_back(i.result);
        else if (i.result->kind == AST::AST_FUNC_DEF)
            Fn.push_back(i.result);
        else if (i.result->kind == AST::AST_INTERFACE)
            Itf.push_back(i.result);
        else if (i.result->kind == AST::AST_IMPORT)
            Import.push_back(i.result);
    }

#ifdef TEST
    registNativeFunction();
#endif

    for (auto i : globalVar) declareGlobalVars(i);

    for (auto i : Class_) {
        auto cls = (Class*)i;
        if (!env->cs.lookup(cls->name)) {
            int clsId = env->classCnt++;
            auto sym = new ClassSymbol(cls->name, nullptr, {},
                                       env->moduleName, clsId, {}, {});
            env->cs.registSymbolGbl(cls->name, sym);
        }
    }

    for (auto i : Fn) {
        auto fn = (Func*)i;
        if (!env->cs.lookup(fn->name)) {
            auto ftp = (FunctionType*)conver->getType(fn->ftype);
            env->cs.registSymbolGbl(fn->name,
                                    new FunctionSymbol(fn->name, ftp, false));
        }
    }

    for (auto i : Itf) {
        auto itf = (Interface*)i;
        if (!env->cs.lookup(itf->name)) {
            auto sym = new InterfaceSymbol(itf->name, {});
            sym->interfaceId = env->interfaceCnt++;
            sym->moduleName  = env->moduleName;
            env->cs.registSymbolGbl(itf->name, sym);
        }
    }
    for (auto i : Itf)   visitInterface(i);
    for (auto i : Class_) visitClass(i);
    for (auto i : Fn)    visitFunction(i, true);

    createTask("@init");
    for (auto i : Import) visitModuleImport(i);
    for (auto i : globalVar) {
        if (i->kind == AST::AST_VAR_DEF)
            visitVarDefine(i, true);
        else if (i->kind == AST::AST_VAR_DEF_GRP)
            visitVarDefineGrp(i, true);
    }

    emit(getLabel(), LOAD_NULL, 0, 0);
    emit(getLabel(), RET, 0, 0);
    endTask(false);

    Module* md = new Module;
    md->globalConstPool = env->globalConstPool;
    return md;
}

int CurrentCompileTask::addLocalVar(std::string name, MType* type, bool isInit) {
    auto sym = new VarSymbol(name, type, isInit, VarSymbol::Local);
    if (currentScope->registVar(name, sym, localVarCnt ))
        return localVarCnt++;
    return -1;
}

int Compiler::addGlobalVar(std::string name, MType *type, bool isInit) {
    auto existing = env->cs.getGlobalScope()->symbols.find(name);
    if (existing != env->cs.getGlobalScope()->symbols.end()
        && existing->second->kind == Symbol::SYM_VAR) {
        return ((VarSymbol*)existing->second)->id;
    }

    auto sym = new VarSymbol(name, type, isInit, VarSymbol::Global);
    if (env->cs.registSymbolGbl(name, sym)) {
        sym->id = env->cs.globalVarCnt;
        return env->cs.globalVarCnt++;
    }
    return -1;
}

void Compiler::declareGlobalVars(AST *a) {
    if (a->kind == AST::AST_VAR_DEF) {
        auto vd = (VarDef*)a;
        auto tp = conver->getType(vd->type);
        addGlobalVar(vd->name, tp, vd->init != nullptr);
    } else if (a->kind == AST::AST_VAR_DEF_GRP) {
        for (auto j : ((VarDefGrp*)a)->vars)
            declareGlobalVars(j);
    }
}

int CompileEnvironment::addFunctionValue(Function *f) {
    globalConstPool.push_back(MicaValue::Object(f));
    return globalConstPool.size()-1;
}

#ifdef TEST
void Compiler::registNativeFunction() {
    std::vector<NativeFunction*> funcs = getFuncs();
    for (auto i : funcs) {
        int idx = env->addFunctionValue(i->nativeFn);
        i->symbol->constPoolIdx = idx;
        env->cs.registSymbolGbl(i->name, i->symbol);
    }
}
#endif