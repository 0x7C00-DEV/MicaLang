#include "../include/compiler.h"
#include "../include/types.h"

namespace {

    MicaValue strToObj(const std::string& s) {
        auto arr = new ObjArray();
        arr->elements.clear();
        for (char c : s)
            arr->elements.push_back(MicaValue::Char(c));
        auto inst = new ObjInstance();
        inst->cls = arr;
        return MicaValue::Object(inst);
    }

} // namespace

AstToMtype::AstToMtype(CompileEnvironment* env, ErrorCollect* ec) {
    this->env = env;
    this->ec = ec;
}

MType* AstToMtype::parseImplementType(AST* a) {
    auto tmp = (ImplementsType*) a;
    std::vector<InterfaceSymbol*> symbols;
    for (auto i : tmp->interfaceNames) {
        auto s = findInterface(i);
        if (!s) return nullptr;
        symbols.push_back((InterfaceSymbol*)s);
    }
    return new TImplementsType(symbols);
}

MType* AstToMtype::parseTemplateType(AST* a) {
    auto tmp = (TemplateType*) a;
    auto root = getType(tmp->rootType);
    if (!root) return nullptr;
    std::vector<MType*> templ;
    for (auto i : tmp->subType) {
        auto t = getType(i);
        if (!t) return nullptr;
        templ.push_back(t);
    }
    return new TTemplateType(root, templ);
}

MType* AstToMtype::getType(AST* a) {
    Type* t = (Type*) a;
    if (t->tpKind == Type::TYPE_ARRAY) return parseArrayType(a);
    if (t->tpKind == Type::TYPE_TEMPLATE) return parseTemplateType(a);
    if (t->tpKind == Type::TYPE_NORMAL) return parseIdNode(a);
    if (t->tpKind == Type::TYPE_FUNC) return parseFuncType(a);
    if (t->tpKind == Type::TYEP_IMPLEMENTS) return parseImplementType(a);
    ec->emitError(new SystemError("Unsupported type kind", a->begin, a->end), false);
    return nullptr;
}

MType* AstToMtype::parseIdNode(AST* a) {
    auto tmp = ((NormalType*)a)->classId;
    return findClassMember(tmp);
}

MType* AstToMtype::parseArrayType(AST* a) {
    auto tmp = (ArrayType*) a;
    auto elementType = getType(tmp->elementType);
    if (!elementType) return nullptr;
    return new TArrayType(elementType);
}

MType* AstToMtype::parseFuncType(AST* a) {
    auto tmp = (FuncType*) a;
    std::vector<MType*> argTypes;
    for (auto i : tmp->args) {
        auto t = getType(i);
        if (!t) return nullptr;
        argTypes.push_back(t);
    }
    auto retType = getType(tmp->retType);
    if (!retType) return nullptr;
    return new FunctionType(retType, argTypes);
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
            ec->emitError(new NameError("Name '" + name + "' is not found", a->begin, a->end), false);
            return nullptr;
        }
        if (sym->kind == Symbol::SYM_INTERFACE)
            return new InterfaceType((InterfaceSymbol*)sym);
        if (sym->kind == Symbol::SYM_CLASS)
            return new ClassType(name, sym);
        if (sym->kind == Symbol::SYM_MODULE)
            return new ModuleType(name, name, sym);
        ec->emitError(new SyntaxError("Name '" + name + "' is not a type", a->begin, a->end), false);
        return nullptr;
    }

    auto parent = ((MemberAccess*)a)->parent;
    auto member = ((MemberAccess*)a)->member;
    auto parentType = findClassMember(parent);
    if (!parentType) return nullptr;

    if (parentType->baseType == MType::BT_MODULE) {
        auto* ms = (ModuleSymbol*)((ModuleType*)parentType)->moduleSymbol;
        auto cit = ms->cls.find(member);
        if (cit != ms->cls.end())
            return new ClassType(member, cit->second);
        auto fit = ms->funcs.find(member);
        if (fit != ms->funcs.end())
            return fit->second->type;
        ec->emitError(new NameError("Module has no member '" + member + "'", a->begin, a->end), false);
        return nullptr;
    }

    if (parentType->baseType != MType::BT_CLASS) {
        ec->emitError(new TypeError("Invalid class member access", a->begin, a->end), false);
        return nullptr;
    }
    auto cls = (ClassSymbol*)((ClassType*)parentType)->sym;
    auto it = cls->members.find(member);
    if (it == cls->members.end()) {
        ec->emitError(new NameError("Class '" + cls->name + "' has no member '" + member + "'", a->begin, a->end), false);
        return nullptr;
    }
    return it->second;
}

Symbol* AstToMtype::findInterface(AST* a) {
    if (a->kind == AST::AST_ID) {
        Symbol* sym = env->cs.lookup(((Id*)a)->name);
        if (!sym) {
            ec->emitError(new NameError("Name '" + ((Id*)a)->name + "' is not found", a->begin, a->end), false);
            return nullptr;
        }
        if (sym->kind != Symbol::SYM_INTERFACE) {
            ec->emitError(new TypeError("Name '" + ((Id*)a)->name + "' is not an interface", a->begin, a->end), false);
            return nullptr;
        }
        return sym;
    }

    if (a->kind != AST::AST_MEMBER_ACCESS) {
        ec->emitError(new SystemError("Unknown type kind", a->begin, a->end), false);
        return nullptr;
    }

    auto tmp = findClassMember(((MemberAccess*)a)->parent);
    if (!tmp || tmp->baseType != MType::BT_CLASS) {
        ec->emitError(new TypeError("Invalid nested interface access", a->begin, a->end), false);
        return nullptr;
    }

    ClassSymbol* cs = ((ClassSymbol*)(((ClassType*)tmp)->sym));
    std::string memberName = ((MemberAccess*)a)->member;

    auto it = cs->members.find(memberName);
    if (it == cs->members.end()) {
        ec->emitError(new TypeError("Class '" + cs->name + "' has no nested interface '" + memberName + "'", a->begin, a->end), false);
        return nullptr;
    }
    if (it->second->baseType != MType::BT_INTERFACE) {
        ec->emitError(new TypeError("Name '" + cs->name + "." + memberName + "' is not an interface", a->begin, a->end), false);
        return nullptr;
    }
    return ((InterfaceType*)it->second)->symbol;
}

ClassSymbol *AstToMtype::findClass(std::string name) {
    auto tmp = (ClassSymbol*)env->cs.lookup(name);
    if (!tmp) {
        ec->emitError(new NameError("Name '" + name + "' not found", {}, {}), false);
        return nullptr;
    }
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

Compiler::Compiler(CompileEnvironment* environment, ErrorCollect* ec): parser(ec) {
    this->env = environment;
    this->ec = ec;
    conver = new AstToMtype(this->env, ec);
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
        ec->emitError(new TypeError("Operator '" + tmp->op + "' is not supported", a->begin, a->end), false);
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
    if (!left) return nullptr;
    MType* right = visitValue(tmp->right, operandExpect);
    if (!right) return nullptr;

    if (left->baseType != MType::BT_ARRAY && left->baseType != MType::BT_CLASS) {
        emit(getLabel(), BIN_OPER, op, 0);
        auto tl = left;
        auto tr = right;
        if (tmp->op == "==" || tmp->op == "!=" || tmp->op == ">=" || tmp->op == "<=" || tmp->op == ">" || tmp->op == "<")
            return new BaseType(BaseType::MBOOL);
        if (typeComp(tl, mica_double) || typeComp(tr, mica_double))
            return new BaseType(BaseType::MDOUBLE);
        return left;
    } else if (left->baseType == MType::BT_CLASS) {
        emit(getLabel(), SWAP_SP, 0, 0);
        emit(getLabel(), DUP, 0, 0);
        auto temp = optofn.find(tmp->op);
        if (temp == optofn.end()) {
            ec->emitError(new TypeError("Operator '" + tmp->op + "' is not supported", a->begin, a->end), false);
            return nullptr;
        }
        auto memberType = ((ClassSymbol*)((ClassType*)left)->sym)->findMember(temp->second);
        if (!memberType || memberType->baseType != MType::BT_FUNC) {
            ec->emitError(new TypeError("Class has no member '" + temp->second + "'", a->begin, a->end), false);
            return nullptr;
        }
        storeString(temp->second);
        emit(getLabel(), MEM_GET, 0, 0);
        emit(getLabel(), SWAP, 0, 2);
        emit(getLabel(), CALL, 2, 0);
        return ((FunctionType*)memberType)->retType;
    } else if (right->baseType == MType::BT_CLASS) {
        emit(getLabel(), DUP, 0, 0);
        auto temp = optofn.find(tmp->op);
        if (temp == optofn.end()) {
            ec->emitError(new TypeError("Operator '" + tmp->op + "' is not supported", a->begin, a->end), false);
            return nullptr;
        }
        auto memberType = ((ClassSymbol*)((ClassType*)right)->sym)->findMember(temp->second);
        if (!memberType || memberType->baseType != MType::BT_FUNC) {
            ec->emitError(new TypeError("Class has no member '" + temp->second + "'", a->begin, a->end), false);
            return nullptr;
        }
        storeString(temp->second);
        emit(getLabel(), MEM_GET, 0, 0);
        emit(getLabel(), SWAP, 0, 2);
        emit(getLabel(), CALL, 2, 0);
        return ((FunctionType*)memberType)->retType;
    } else {
        ec->emitError(new TypeError("Unsupported operand types for binary operator '" + tmp->op + "'", a->begin, a->end), false);
        return nullptr;
    }
}

Symbol *Compiler::getMemberType(AST* node) {
    if (node->kind == AST::AST_ID) {
        Symbol* sym = env->cs.lookup(((Id*)node)->name);
        if (!sym) {
            ec->emitError(new NameError("Symbol '" + ((Id*)node)->name + "' not found", node->begin, node->end), false);
            return nullptr;
        }
        return sym;
    }
    if (node->kind == AST::AST_MEMBER_ACCESS) {
        auto parent = getMemberType(((MemberAccess*)node)->parent);
        if (!parent) return nullptr;
        auto member = ((MemberAccess*)node)->member;

        if (parent->kind == Symbol::SYM_MODULE) {
            auto* ms = (ModuleSymbol*)parent;
            auto fit = ms->funcs.find(member);
            if (fit != ms->funcs.end()) return fit->second;
            auto cit = ms->cls.find(member);
            if (cit != ms->cls.end()) return cit->second;
            ec->emitError(new NameError("Module has no member '" + member + "'", node->begin, node->end), false);
            return nullptr;
        }

        if (parent->kind != Symbol::SYM_CLASS) {
            ec->emitError(new TypeError("Not a class", node->begin, node->end), false);
            return nullptr;
        }
        auto it = ((ClassSymbol*)parent)->syms.find(member);
        if (it == ((ClassSymbol*)parent)->syms.end()) {
            ec->emitError(new NameError("Class '" + ((ClassSymbol*)parent)->name + "' has no member '" + member + "'", node->begin, node->end), false);
            return nullptr;
        }
        return it->second;
    }
    if (node->kind == AST::AST_ELEMENT_GET) {
        auto tmp = (ElementGet*) node;
        return getMemberType(tmp->address);
    }
    if (node->kind == AST::AST_CALL) {
        auto tmp = (Call*) node;
        return getMemberType(tmp->fnid);
    }
    ec->emitError(new SystemError("Unsupported member kind", node->begin, node->end), false);
    return nullptr;
}

MType* Compiler::visitCallNode(AST* a, MType* expect) {
    auto tmp = (Call*) a;
    MType* tp1 = visitValue(tmp->fnid, expect);

    if (!tp1) {
        ec->emitError(new TypeError("Cannot resolve callee type", a->begin, a->end), false);
        return nullptr;
    }

    if (tp1->baseType == MType::BT_FUNC) {
        auto tp = (FunctionType*) tp1;
        int argc = (int)tmp->args.size();

        if (tmp->fnid->kind == AST::AST_MEMBER_ACCESS) {
            auto temp = getMemberType(((MemberAccess*)tmp->fnid)->parent);
            if (temp && temp->kind != Symbol::SYM_CLASS && temp->kind != Symbol::SYM_MODULE) {
                auto recvType = visitValue(((MemberAccess*)tmp->fnid)->parent, nullptr);
                if (!recvType) return nullptr;
                argc += 1;
            }
        }

        for (int i = 0; i < (int)tmp->args.size(); ++i) {
            MType* expectedArg = (i < (int)tp->argsType.size()) ? tp->argsType[i] : nullptr;
            MType* actualArg = visitValue(tmp->args[i], expectedArg);
            if (!actualArg) return nullptr;
            if (expectedArg && !typeComp(actualArg, expectedArg)) {
                ec->emitError(new TypeError("Argument " + std::to_string(i + 1) + ": expected '" + getTypeString(expectedArg) + "', got '" + getTypeString(actualArg) + "'", tmp->args[i]->begin, tmp->args[i]->end), false);
                return nullptr;
            }
        }

        emit(getLabel(), CALL, argc, 0);
        return tp->retType;

    } else if (tp1->baseType == MType::BT_CLASS) {
        auto csym = ((ClassSymbol*)((ClassType*) tp1)->sym);

        MType* callFnType = csym->findMember("__call__");
        if (!callFnType) {
            ec->emitError(new TypeError("Class '" + csym->name + "' has no member '__call__'", a->begin, a->end), false);
            return nullptr;
        }
        if (callFnType->baseType != MType::BT_FUNC) {
            ec->emitError(new TypeError("'__call__' is not a function", a->begin, a->end), false);
            return nullptr;
        }
        auto ftmp = (FunctionType*) callFnType;
        auto retTp = ftmp->retType;

        emit(getLabel(), DUP, 0, 0);
        storeString("__call__");
        emit(getLabel(), MEM_GET, 0, 0);
        emit(getLabel(), SWAP_SP, 0, 0);

        for (int i = 0; i < (int)tmp->args.size(); ++i) {
            MType* expectedArg = (i + 1 < (int)ftmp->argsType.size()) ? ftmp->argsType[i + 1] : nullptr;
            MType* actualArg = visitValue(tmp->args[i], expectedArg);
            if (!actualArg) return nullptr;
            if (expectedArg && !typeComp(actualArg, expectedArg)) {
                ec->emitError(new TypeError("Argument " + std::to_string(i + 1) + ": expected '" + getTypeString(expectedArg) + "', got '" + getTypeString(actualArg) + "'", tmp->args[i]->begin, tmp->args[i]->end), false);
                return nullptr;
            }
        }
        emit(getLabel(), CALL, (int)tmp->args.size() + 1, 0);
        return retTp;

    } else {
        ec->emitError(new TypeError("Type cannot be called", a->begin, a->end), false);
        return nullptr;
    }
}

MType* Compiler::visitElementGet(AST* a, MType* expect) {
    auto arrayId = (ElementGet*)a;
    auto* tpm = visitValue(arrayId->address, expect);

    if (!tpm) {
        ec->emitError(new TypeError("Cannot resolve address type", a->begin, a->end), false);
        return nullptr;
    }

    if (tpm->baseType == MType::BT_ARRAY) {
        auto tp = (TArrayType*) tpm;

        MType* posType = visitValue(arrayId->position, mica_int);
        if (!posType) return nullptr;
        emit(getLabel(), EL_GET, 0, 0);
        if (!typeComp(posType, mica_int)) {
            ec->emitError(new TypeError("Array index must be int, got '" + getTypeString(posType) + "'", arrayId->position->begin, arrayId->position->end), false);
            return nullptr;
        }
        return tp->elementType;

    } else if (tpm->baseType == MType::BT_CLASS) {
        ClassSymbol* cls = ((ClassSymbol*)((ClassType*) tpm)->sym);

        MType* elemGetType = cls->findMember("__elementget__");
        if (!elemGetType) {
            ec->emitError(new TypeError("Object '" + cls->name + "' has no member '__elementget__'", a->begin, a->end), false);
            return nullptr;
        }
        if (elemGetType->baseType != MType::BT_FUNC) {
            ec->emitError(new TypeError("'__elementget__' is not callable", a->begin, a->end), false);
            return nullptr;
        }
        auto ft = (FunctionType*) elemGetType;
        MType* retT = ft->retType;

        emit(getLabel(), DUP, 0, 0);
        storeString("__elementget__");
        emit(getLabel(), MEM_GET, 0, 0);
        emit(getLabel(), SWAP, 0, 1);

        MType* expectedPos = (ft->argsType.size() >= 2) ? ft->argsType[1] : mica_int;
        MType* actualPos = visitValue(arrayId->position, expectedPos);
        if (!actualPos) return nullptr;
        if (expectedPos && !typeComp(actualPos, expectedPos)) {
            ec->emitError(new TypeError("'__elementget__' index type mismatch: expected '" + getTypeString(expectedPos) + "', got '" + getTypeString(actualPos) + "'", arrayId->position->begin, arrayId->position->end), false);
            return nullptr;
        }

        emit(getLabel(), CALL, 2, 0);
        return retT;

    } else {
        ec->emitError(new TypeError("Type cannot be indexed", a->begin, a->end), false);
        return nullptr;
    }
}

MType* Compiler::visitMemberAccess(AST* a, MType* expect) {
    auto tmp = (MemberAccess*) a;
    auto parent = visitValue(tmp->parent, expect);
    if (!parent) {
        ec->emitError(new TypeError("Cannot resolve parent type", a->begin, a->end), false);
        return nullptr;
    }

    if (parent->baseType == MType::BT_MODULE) {
        auto* ms = (ModuleSymbol*)((ModuleType*)parent)->moduleSymbol;

        auto fit = ms->funcs.find(tmp->member);
        if (fit != ms->funcs.end()) {
            int idx = pushConstL(strToObj(tmp->member));
            emit(getLabel(), LOAD_MODULE_MEMBER, idx, 0);
            return fit->second->type;
        }
        auto cit = ms->cls.find(tmp->member);
        if (cit != ms->cls.end()) {
            int idx = pushConstL(strToObj(tmp->member));
            emit(getLabel(), LOAD_MODULE_MEMBER, idx, 0);
            return new ClassType(tmp->member, cit->second);
        }
        ec->emitError(new NameError("Module has no member '" + tmp->member + "'", a->begin, a->end), false);
        return nullptr;
    }

    storeString(tmp->member);
    emit(getLabel(), MEM_GET, 0, 0);

    if (parent->baseType == MType::BT_INTERFACE) {
        auto it = ((InterfaceType*)parent)->symbol->labels.find(tmp->member);
        if (it == ((InterfaceType*)parent)->symbol->labels.end()) {
            ec->emitError(new NameError("Interface '" + ((InterfaceType*)parent)->symbol->name + "' has no member '" + tmp->member + "'", a->begin, a->end), false);
            return nullptr;
        }
        return it->second->type;
    }

    if (parent->baseType != MType::BT_CLASS) {
        ec->emitError(new TypeError("Member access on non-class type", a->begin, a->end), false);
        return nullptr;
    }

    auto cls = ((ClassSymbol*)((ClassType*)parent)->sym);
    MType* memberType = cls->findMember(tmp->member);
    if (!memberType) {
        ec->emitError(new NameError("Class '" + cls->name + "' (or its super classes) has no member '" + tmp->member + "'", a->begin, a->end), false);
        return nullptr;
    }
    return memberType;
}

void Compiler::visitVarDefineGrp(AST* a, bool isGlobal) {
    auto tmp = (VarDefGrp*) a;
    for (auto i : tmp->vars)
        visitVarDefine(i, isGlobal);
}

void Compiler::visitForIn(AST *a) {
    createScope(Scope::SNORMAL_BLOCK);
    ForIn* f = (ForIn*)a;
    for (auto i : f->expand)
        visitStmt(i, "", "");
    leaveScope();
}

Vresult Compiler::visitVarDefine(AST* a, bool isGlobal) {
    Vresult res;
    auto tmp = (VarDef *) a;
    MType* tp = (tmp->type)? conver->getType(tmp->type) : nullptr;
    if (tmp->type && !tp) return res;

    if (!isGlobal) {
        if (!tp && !tmp->init) {
            ec->emitError(new TypeError("Cannot infer the type of variable '" + tmp->name + "'", a->begin, a->end), false);
            return res;
        }

        int varId = getCurrentTsk()->addLocalVar(tmp->name, tp,
                                                 tmp->init != nullptr);

        if (tmp->init) {
            MType* initType = visitValue(tmp->init, tp);
            if (!initType) return res;

            if (!tp) {
                tp = initType;
                auto sym = getCurrentTsk()->currentScope
                              ->lookupLocalVar(tmp->name);
                if (sym && sym->kind == Symbol::SYM_VAR)
                    ((VarSymbol*)sym)->type = tp;
            } else if (!typeComp(initType, tp)) {
                ec->emitError(new TypeError("Variable '" + tmp->name + "': expected type '" + getTypeString(tp) + "', got '" + getTypeString(initType) + "'", a->begin, a->end), false);
                return res;
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
        ec->emitError(new TypeError("Cannot infer the type of variable '" + tmp->name + "'", a->begin, a->end), false);
        return res;
    }
    if (tmp->init) {
        MType* initType = visitValue(tmp->init, tp);
        if (!initType) return res;
        if (!tp) {
            tp = initType;
            auto sym = env->cs.getGlobalScope()->symbols[tmp->name];
            if (sym && sym->kind == Symbol::SYM_VAR)
                ((VarSymbol*)sym)->type = tp;
        } else if (!typeComp(initType, tp)) {
            ec->emitError(new TypeError("Variable '" + tmp->name + "': expected type '" + getTypeString(tp) + "', got '" + getTypeString(initType) + "'", a->begin, a->end), false);
            return res;
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
    std::string moduleName = tmp->moduleName;
    SymLoader loader(moduleName);
    ModuleSymbol* sym = loader.getModuleTag(moduleName);
    if (!sym) {
        ec->emitError(new NameError("Module '" + moduleName + "' cannot be loaded", a->begin, a->end), false);
        return nullptr;
    }
    env->cs.registSymbolGbl(moduleName, sym);
    return new ModuleType(moduleName, moduleName, sym);
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
    if (!ftp) {
        env->tasks.pop_back();
        env->cs.leaveScope();
        return nullptr;
    }
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
    ec->emitError(new SystemError("Unsupported expression kind", a->begin, a->end), false);
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
            ec->emitError(new NameError("Name '" + tmpx->name + "' not found", dst->begin, dst->end), false);
            return nullptr;
        }
        if (s->kind != Symbol::SYM_VAR) {
            ec->emitError(new TypeError("Name '" + tmpx->name + "' is not a variable", dst->begin, dst->end), false);
            return nullptr;
        }
        auto temp = (VarSymbol*) s;
        auto vt = visitValue(src, temp->type);
        if (!vt) return nullptr;
        auto st = temp->type;
        if (!typeComp(vt, st)) {
            ec->emitError(new TypeError("Variable '" + tmpx->name + "': expected '" + getTypeString(st) + "', got '" + getTypeString(vt) + "'", a->begin, a->end), false);
            return nullptr;
        }
        int oper = (temp->vkind == VarSymbol::Global)? STORE_GVAR : STORE_SVAR;
        emit(getLabel(), oper, temp->id, 0);
        return temp->type;
    }
    if (dst->kind == AST::AST_ELEMENT_GET) {
        auto temp = (ElementGet*) dst;
        MType* obj = visitValue(temp->address, nullptr);
        if (!obj) return nullptr;
        if (obj->baseType == MType::BT_ARRAY) {
            MType* valt = visitValue(src, ((TArrayType*)obj)->elementType);
            if (!valt) return nullptr;
            MType* pos_ = visitValue(temp->position, mica_int);
            if (!pos_) return nullptr;
            emit(getLabel(), EL_SET, 0, 0);
            if (!typeComp(pos_, mica_int)) {
                ec->emitError(new TypeError("Array index must be int, got '" + getTypeString(pos_) + "'", temp->position->begin, temp->position->end), false);
                return nullptr;
            }
            if (!typeComp(valt, ((TArrayType*)obj)->elementType)) {
                ec->emitError(new TypeError("Array element type is '" + getTypeString(((TArrayType*)obj)->elementType) + "', value type is '" + getTypeString(valt) + "'", a->begin, a->end), false);
                return nullptr;
            }
            return valt;
        } else if (obj->baseType == MType::BT_CLASS) {
            ClassSymbol* cls = ((ClassSymbol*)((ClassType*) obj)->sym);
            auto it = cls->members.find("__elementset__");
            if (it == cls->members.end()) {
                ec->emitError(new TypeError("Object '" + cls->name + "' has no member '__elementset__'", a->begin, a->end), false);
                return nullptr;
            }
            if (it->second->baseType != MType::BT_FUNC) {
                ec->emitError(new TypeError("'__elementset__' is not callable", a->begin, a->end), false);
                return nullptr;
            }
            auto ft = (FunctionType*)it->second;

            emit(getLabel(), DUP, 0, 0);
            storeString("__elementset__");
            emit(getLabel(), MEM_GET, 0, 0);
            emit(getLabel(), SWAP_SP, 0, 0);
            auto posType = visitValue(temp->position, ft->argsType[1]);
            if (!posType) return nullptr;
            auto valType = visitValue(src, ft->argsType[2]);
            if (!valType) return nullptr;
            emit(getLabel(), CALL, 3, 0);
            emit(getLabel(), POP, 0, 0);
            return ft->retType;
        } else {
            ec->emitError(new TypeError("Element assignment on unsupported type", a->begin, a->end), false);
            return nullptr;
        }
    }
    if (dst->kind == AST::AST_MEMBER_ACCESS) {
        auto tmpx = (MemberAccess*) dst;
        MType* parentType = visitValue(tmpx->parent, nullptr);
        if (!parentType) return nullptr;
        if (parentType->baseType != MType::BT_CLASS) {
            ec->emitError(new TypeError("Member assignment on non-class type", a->begin, a->end), false);
            return nullptr;
        }
        auto cls = ((ClassSymbol*)((ClassType*)parentType)->sym);
        MType* memberType = cls->findMember(tmpx->member);
        if (!memberType) {
            ec->emitError(new NameError("Class '" + cls->name + "' has no member '" + tmpx->member + "'", dst->begin, dst->end), false);
            return nullptr;
        }
        MType* valueType = visitValue(src, memberType);
        if (!valueType) return nullptr;
        storeString(tmpx->member);
        emit(getLabel(), MEM_SET, 0, 0);
        return valueType;
    }
    ec->emitError(new SystemError("Unsupported assignment target", dst->begin, dst->end), false);
    return nullptr;
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
    if (a->kind == AST::AST_FOR_IN) {
        visitForIn(a);
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
    MType* t = visitValue(a, nullptr);
    if (!t) return nullptr;
    if (a->kind != AST::AST_ASSIGN_NODE)
        emit(getLabel(), POP, 0, 0);
    return t;
}

MType* Compiler::visitTernOp(AST* a, MType* expect) {
    std::string false_ = getLabel();
    std::string end = getLabel();
    MType* retTypet, * retTypef;
    auto tmp = (ThreeOp*)a;
    auto tp = visitValue(tmp->condition, expect);
    if (!tp) return nullptr;
    emit(getLabel(), JMPF, false_, 0);
    retTypet = visitValue(tmp->trueValue, expect);
    if (!retTypet) return nullptr;
    emit(getLabel(), JMP, end, 0);
    emit(false_, NOP, 0, 0);
    retTypef = visitValue(tmp->falseValue, expect);
    if (!retTypef) return nullptr;
    emit(end, NOP, 0, 0);
    if (!typeComp(retTypef, retTypet)) {
        ec->emitError(new TypeError("Ternary branches type mismatch: '" + getTypeString(retTypet) + "' vs '" + getTypeString(retTypef) + "'", a->begin, a->end), false);
        return nullptr;
    }
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
        if (!tp) {
            leaveScope();
            return nullptr;
        }
        if (!typeComp(tp, mica_bool) && !typeComp(tp, mica_int)) {
            ec->emitError(new TypeError("For loop condition: expected bool, got '" + getTypeString(tp) + "'", tmp->condition->begin, tmp->condition->end), false);
            leaveScope();
            return nullptr;
        }
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

    if (tp && !typeComp(tp, mica_bool) && !typeComp(tp, mica_int)) {
        ec->emitError(new TypeError("For loop condition: expected bool, got '" + getTypeString(tp) + "'", tmp->condition->begin, tmp->condition->end), false);
        leaveScope();
        return nullptr;
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
    if (!tp) {
        leaveScope();
        return nullptr;
    }
    emit(getLabel(), JMPF, end, 0);
    visitBlock(tmp->body, begin, end);
    emit(getLabel(), JMP, begin, 0);
    emit(end, NOP, 0,0);
    if (!typeComp(tp, mica_bool) && !typeComp(tp, mica_int)) {
        ec->emitError(new TypeError("While loop condition: expected bool, got '" + getTypeString(tp) + "'", tmp->condition->begin, tmp->condition->end), false);
        leaveScope();
        return nullptr;
    }
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
    if (!tp) {
        leaveScope();
        return nullptr;
    }
    if (!typeComp(tp, mica_bool) && !typeComp(tp, mica_int)) {
        ec->emitError(new TypeError("Do-while condition: expected bool, got '" + getTypeString(tp) + "'", tmp->condition->begin, tmp->condition->end), false);
        leaveScope();
        return nullptr;
    }
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
    if (!tp) {
        leaveScope();
        return nullptr;
    }
    auto t = tp;
    std::vector<MType*> types;
    emit(begin, NOP, 0, 0);
    for (auto i: tmp->cases) {
        auto su = (Case*) i;
        if (su->value) {
            createScope(Scope::SNORMAL_BLOCK);
            std::string end_ = getLabel();
            emit(getLabel(), DUP, 0, 0);
            auto caseType = visitValue(su->value, tp);
            if (!caseType) {
                leaveScope();
                leaveScope();
                return nullptr;
            }
            types.push_back(caseType);
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
    for (auto i: types) {
        if (!typeComp(i, t)) {
            ec->emitError(new TypeError("Switch case type '" + getTypeString(i) + "' does not match switch value type '" + getTypeString(t) + "'", a->begin, a->end), false);
            leaveScope();
            return nullptr;
        }
    }
    leaveScope();
    return nullptr;
}

MType* Compiler::visitInterface(AST* a) {
    Interface* i = (Interface*)a;
    auto sym = (InterfaceSymbol*)env->cs.lookup(i->name);
    if (!sym || sym->kind != Symbol::SYM_INTERFACE) {
        ec->emitError(new NameError("Interface '" + i->name + "' is not registered", a->begin, a->end), false);
        return nullptr;
    }

    for (auto j : i->funcs) {
        auto temp = (Interface::FunctionTag*) j;
        auto ft = (FunctionType*)conver->getType(temp->funcType);
        if (!ft) return nullptr;
        sym->labels[temp->name] = new FunctionSymbol(temp->name, ft, false);
    }
    return nullptr;
}

MType* Compiler::visitClass(AST* a) {
    auto cls = (Class*) a;
    auto tmp = new ObjClass(cls->name);

    ClassSymbol* super = (ClassSymbol*)getClassInfo(cls->extend);
    std::vector<InterfaceSymbol*> impls;
    for (auto i : cls->impls) {
        auto tmp1 = getInterface(i);
        if (!tmp1) {
            ec->emitError(new NameError("Interface cannot be resolved", i->begin, i->end), false);
            return nullptr;
        }
        if (tmp1->kind != Symbol::SYM_INTERFACE) {
            ec->emitError(new TypeError("Symbol is not an interface", i->begin, i->end), false);
            return nullptr;
        }
        impls.push_back((InterfaceSymbol*)tmp1);
    }

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
            auto mt = conver->getType(vd->type);
            if (!mt) return nullptr;
            sym->members[vd->name] = mt;
            sym->syms[vd->name] = new VarSymbol(vd->name, mt, false, VarSymbol::ClassMember);
        } else if (i->kind == AST::AST_VAR_DEF_GRP) {
            for (auto j : ((VarDefGrp*)i)->vars) {
                auto vd = (VarDef*)j;
                fields.push_back(vd->name);
                auto mt = conver->getType(vd->type);
                if (!mt) return nullptr;
                sym->members[vd->name] = mt;
                sym->syms[vd->name] = new VarSymbol(vd->name, mt, false, VarSymbol::ClassMember);
            }
        } else if (i->kind == AST::AST_INTERFACE) {
            auto inf = (Interface*) i;
            std::unordered_map<std::string, FunctionSymbol*> tags;
            for (auto k : inf->funcs) {
                auto fs = visitFuncTag(k);
                if (!fs) return nullptr;
                tags[((Interface::FunctionTag*)k)->name] = fs;
            }
            sym->syms[inf->name] = new InterfaceSymbol(inf->name, tags);
            sym->members[inf->name] = new InterfaceType((InterfaceSymbol*)sym->syms[inf->name]);
        }
    }
    tmp->fields = fields;
    for (auto i : cls->methods) {
        if (i->kind == AST::AST_FUNC_DEF) {
            auto fn = (Func*)i;
            auto mt = conver->getType(fn->ftype);
            if (!mt) return nullptr;
            sym->members[fn->name] = mt;
            sym->syms[fn->name] = new FunctionSymbol(fn->name, (FunctionType*)mt, true);
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
    std::string className = tmp->className;
    auto dotPos = className.find('.');

    MType* retType = nullptr;

    if (dotPos != std::string::npos) {
        std::string modName = className.substr(0, dotPos);
        std::string clsName = className.substr(dotPos + 1);

        auto* sym = env->cs.lookup(modName);
        if (!sym || sym->kind != Symbol::SYM_MODULE) {
            ec->emitError(new NameError("'" + modName + "' is not a module", a->begin, a->end), false);
            return nullptr;
        }
        auto* ms = (ModuleSymbol*)sym;
        auto it = ms->cls.find(clsName);
        if (it == ms->cls.end()) {
            ec->emitError(new NameError("Module '" + modName + "' has no class '" + clsName + "'", a->begin, a->end), false);
            return nullptr;
        }
        retType = new ClassType(clsName, it->second);

        int modIdx = pushConstL(strToObj(modName));
        int clsIdx = pushConstL(strToObj(clsName));
        emit(getLabel(), LOAD_MODULE, modIdx, 0);
        emit(getLabel(), LOAD_MODULE_MEMBER, clsIdx, 0);
        emit(getLabel(), NEW, 0, 0);
    } else {
        int pos = -1;
        for (int i = 0; i < (int)env->globalConstPool.size(); ++i) {
            auto k = env->globalConstPool[i];
            if (k.kind == MicaValue::OBJ && k.obj->tp == Obj::USER_DEFING_CLASS &&
                ((ObjClass*)k.obj)->name == className) {
                pos = i;
                break;
            }
        }
        if (pos == -1) {
            ec->emitError(new NameError("Class '" + className + "' not found", a->begin, a->end), false);
            return nullptr;
        }
        retType = new ClassType(className, env->cs.lookup(className));
        emit(getLabel(), LOAD_GCST, pos, 0);
        emit(getLabel(), NEW, 0, 0);
    }

    if (tmp->isCallInit) {
        emit(getLabel(), DUP, 0, 0);
        storeString("__init__");
        emit(getLabel(), MEM_GET, 0, 0);
        emit(getLabel(), DUP, 1, 0);
        for (auto i : tmp->initArgs) {
            auto argType = visitValue(i, nullptr);
            if (!argType) return nullptr;
        }
        emit(getLabel(), CALL, (int)tmp->initArgs.size() + 1, 0);
        emit(getLabel(), POP, 0, 0);
    }
    return retType;
}

MType* Compiler::visitArray(AST* a, MType* type) {
    auto tmp = (Array*)a;
    int n = (int)tmp->elements.size();
    loadConstS(pushConstL(MicaValue::Int(n)));
    emit(getLabel(), NEW_ARR, 0, 0);
    MType* elemType = nullptr;
    if (type) {
        if (type->baseType != MType::BT_ARRAY) {
            ec->emitError(new TypeError("Expected array type", a->begin, a->end), false);
            return nullptr;
        }
        elemType = ((TArrayType*)type)->elementType;
    }
    for (int i = 0; i < n; ++i) {
        emit(getLabel(), DUP, 0, 0);
        MType* it = visitValue(tmp->elements[i], elemType);
        if (!it) return nullptr;
        loadConstS(pushConstL(MicaValue::Int(i)));
        if (!elemType) elemType = it;
        else if (!typeComp(elemType, it)) {
            ec->emitError(new TypeError("Array element type mismatch: '" + getTypeString(it) + "' vs '" + getTypeString(elemType) + "'", tmp->elements[i]->begin, tmp->elements[i]->end), false);
            return nullptr;
        }
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
        if (!tp) return nullptr;
    } else {
        emit(getLabel(), LOAD_NULL, 0, 0);
    }
    emit(getLabel(), RET, 0, 0);
    return tp;
}

FunctionSymbol *Compiler::visitFuncTag(AST *f) {
    auto symast = (Interface::FunctionTag*) f;
    auto ftype = (FunctionType*)conver->getType((FuncType*) symast->funcType);
    if (!ftype) return nullptr;
    return new FunctionSymbol(symast->name, ftype, false);
}

MType *Compiler::visitId(AST *a) {
    auto name = ((Id*)a)->name;
    auto info = getCurrentTsk()->currentScope->lookupSymbol(name);
    if (!info) {
        ec->emitError(new NameError("Identifier '" + name + "' is not found", a->begin, a->end), false);
        return nullptr;
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
        int idx = pushConstL(strToObj(name));
        emit(getLabel(), LOAD_MODULE, idx, 0);
        return new ModuleType(name, name, info);
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
        ec->emitError(new NameError("Class '" + name + "' not found in global const pool", a->begin, a->end), false);
        return nullptr;
    }

    ec->emitError(new SystemError("Unsupported symbol kind", a->begin, a->end), false);
    return nullptr;
}

MType *Compiler::visitNeg(AST *a, MType* expect) {
    auto tmp = (Neg*)a;
    auto val = visitValue(tmp->value, expect);
    if (!val) return nullptr;
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
    if (!tmp) return nullptr;
    emit(getLabel(), BBNOT, 0, 0);
    return tmp;
}

MType *Compiler::visitNot(AST *pAst, MType* expect) {
    auto bn = (Not*) pAst;
    auto tmp = visitValue(bn->value, expect);
    if (!tmp) return nullptr;
    emit(getLabel(), BNOT, 0, 0);
    return new BaseType(BaseType::MBOOL);
}

MType *Compiler::visitIf(AST *a, std::string begin, std::string end) {
    createScope(Scope::SNORMAL_BLOCK);
    auto tmp = (If*)a;
    std::string if_ = getLabel();
    std::string ie = getLabel();
    auto tp = visitValue(tmp->condition, new BaseType(BaseType::MBOOL));
    if (!tp) {
        leaveScope();
        return nullptr;
    }
    if (!typeComp(tp, mica_bool)) {
        ec->emitError(new TypeError("If condition: expected bool, got '" + getTypeString(tp) + "'", tmp->condition->begin, tmp->condition->end), false);
        leaveScope();
        return nullptr;
    }
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

Symbol *Compiler::getClassInfo(std::string name) {
    if (name.empty()) return nullptr;
    auto cls = env->cs.lookup(name);
    if (!cls) return nullptr;
    if (cls->kind != Symbol::SYM_CLASS) return nullptr;
    if (((ClassSymbol*)cls)->name != name) return nullptr;
    return (ClassSymbol*)cls;
}

Symbol *Compiler::getInterface(AST* tname) {
    if (!tname) return nullptr;
    if (tname->kind == AST::AST_ID) {
        auto name = ((Id*)tname)->name;
        auto cls = env->cs.lookup(name);
        if (!cls) {
            ec->emitError(new NameError("Name '" + name + "' not found", tname->begin, tname->end), false);
            return nullptr;
        }
        if (cls->kind != Symbol::SYM_INTERFACE) {
            ec->emitError(new TypeError("Name '" + name + "' is not an interface", tname->begin, tname->end), false);
            return nullptr;
        }
        return cls;
    }
    if (tname->kind == AST::AST_MEMBER_ACCESS) {
        auto h = getInterface(((MemberAccess*)tname)->parent);
        if (!h) return nullptr;
        if (h->kind != Symbol::SYM_CLASS) {
            ec->emitError(new TypeError("Nested interface access on non-class symbol", tname->begin, tname->end), false);
            return nullptr;
        }
        auto parent = (ClassSymbol*)h;
        auto member = ((MemberAccess*)tname)->member;
        auto it = parent->syms.find(member);
        if (it == parent->syms.end()) {
            ec->emitError(new NameError("Class '" + parent->name + "' has no nested symbol '" + member + "'", tname->begin, tname->end), false);
            return nullptr;
        }
        return it->second;
    }
    ec->emitError(new SystemError("Unknown interface kind", tname->begin, tname->end), false);
    return nullptr;
}

int Compiler::getInterfaceId(AST* name) {
    auto temp = getInterface(name);
    if (!temp) return -1;
    if (temp->kind != Symbol::SYM_INTERFACE) {
        ec->emitError(new TypeError("Symbol is not an interface", name->begin, name->end), false);
        return -1;
    }
    auto tmp = (InterfaceSymbol*)temp;
    return tmp->interfaceId;
}

Module* Compiler::getProgram(std::string text, std::string fileName, ErrorCollect* ec) {
    this->ec = ec;
    size_t mark = ec->errors.size();
    std::string moduleName;

    std::vector<Register> tmp = parser.parseCode(text, fileName);
    for (auto i : tmp)
        if (!i.isSuc)
            return nullptr;

    std::vector<AST*> Class_, globalVar, Fn, Itf, Import, Other;
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
        else if (i.result->kind == AST::AST_MODULE_NAME)
        {
            moduleName = ((ModuleName*)i.result)->moduleName;
        }
        else Other.push_back(i.result);
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

    for (auto i : Itf) {
        auto itf = (Interface*)i;
        if (!env->cs.lookup(itf->name)) {
            auto sym = new InterfaceSymbol(itf->name, {});
            sym->interfaceId = env->interfaceCnt++;
            sym->moduleName  = env->moduleName;
            env->cs.registSymbolGbl(itf->name, sym);
        }
    }

    for (auto i : Fn) {
        auto fn = (Func*)i;
        if (!env->cs.lookup(fn->name)) {
            auto ftp = (FunctionType*)conver->getType(fn->ftype);
            if (!ftp) return nullptr;
            env->cs.registSymbolGbl(fn->name,
                                    new FunctionSymbol(fn->name, ftp, false));
        }
    }

    for (auto i : Itf)   visitInterface(i);
    for (auto i : Class_) visitClass(i);
    for (auto i : Fn)    visitFunction(i, true);

    if (!env->cs.lookup("@init")) {
        env->cs.registSymbolGbl(
            "@init",
            new FunctionSymbol("@init",
                               new FunctionType(mica_void, {}),
                               false));
    }

    createTask("@init");
    for (auto i : Import) visitModuleImport(i);
    for (auto i : globalVar) {
        if (i->kind == AST::AST_VAR_DEF)
            visitVarDefine(i, true);
        else if (i->kind == AST::AST_VAR_DEF_GRP)
            visitVarDefineGrp(i, true);
    }

    for (auto i : Other)
        visitStmt(i, "", "");

    emit(getLabel(), LOAD_NULL, 0, 0);
    emit(getLabel(), RET, 0, 0);
    endTask(false);

    if (ec->errors.size() > mark) return nullptr;

    Module* md = new Module;
    md->globalConstPool = env->globalConstPool;
    md->moduleName = moduleName;
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
        MType* tp = vd->type ? conver->getType(vd->type) : nullptr;
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