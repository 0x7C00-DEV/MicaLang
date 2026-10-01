#include "../include/dis.h"
#include <iomanip>

std::string getOperator(int op) {
    auto tmp = operMap.find(op);
    if (tmp == operMap.end())
        return "UNK_INS" + std::to_string(op);
    return tmp->second;
}

std::string getBinOper(int bop) {
    auto tmp = binOpMap.find(bop);
    if (tmp == binOpMap.end())
        return "UNK_OP" + std::to_string(bop);
    return tmp->second;
}

Dis::Dis(Module* module) {
    this->module = module;
}

void Dis::showConstValue(const MicaValue& v) {
    switch (v.kind) {
        case MicaValue::INT:
            std::cout << "INT(" << v.i << ")";
            break;
        case MicaValue::FLOAT:
            std::cout << "FLOAT(" << v.f << ")";
            break;
        case MicaValue::MBOOL:
            std::cout << "BOOL(" << (v.b ? "true" : "false") << ")";
            break;
        case MicaValue::NUL:
            std::cout << "NULL";
            break;
        case MicaValue::CHAR: {
            char c = v.c;
            switch (c) {
                case '\n': std::cout << "CHAR('\\n')"; break;
                case '\t': std::cout << "CHAR('\\t')"; break;
                case '\r': std::cout << "CHAR('\\r')"; break;
                case '\0': std::cout << "CHAR('\\0')"; break;
                case '\'': std::cout << "CHAR('\\'')"; break;
                case '\\': std::cout << "CHAR('\\\\')"; break;
                default:   std::cout << "CHAR('" << c << "')"; break;
            }
            break;
        }
        case MicaValue::OBJ:
            switch (v.obj->tp) {
                case Obj::FUNCTION: {
                    auto f = (Function*)v.obj;
                    std::cout << "FUNCTION(" << f->name;
                    if (f->isNative) std::cout << ", native";
                    std::cout << ", consts=" << f->constants.size()
                              << ", code=" << f->ins.size() << ")";
                    break;
                }
                case Obj::USER_DEFING_CLASS: {
                    auto c = (ObjClass*)v.obj;
                    std::cout << "CLASS(" << c->name
                              << ", fields=" << c->fields.size()
                              << ", methods=" << c->methods.size();
                    if (c->super) std::cout << ", super=" << c->super->name;
                    std::cout << ")";
                    break;
                }
                case Obj::INITED_OBJECT: {
                    auto ins = (ObjInstance*)v.obj;
                    std::cout << "INSTANCE("
                              << (ins->cls ? ins->cls->name : "?") << ")";
                    break;
                }
                case Obj::MODULE: {
                    auto m = (Module*)v.obj;
                    std::cout << "MODULE(" << m->moduleName << ")";
                    break;
                }
            }
            break;
    }
}

void Dis::showConstPool(Function* fn) {
    auto& cpl = fn->constants;
    std::cout << "  constants (" << cpl.size() << "):\n";
    for (int i = 0; i < (int)cpl.size(); ++i) {
        std::cout << "    " << i << ": ";
        showConstValue(cpl[i]);
        std::cout << "\n";
    }
}

void Dis::showInstruation(Function* fn) {
    auto& ins = fn->ins;
    for (int i = 0; i < (int)ins.size(); ++i) {
        Instr is = decodeInstr(ins[i]);
        std::cout << "    " << i << ":  ";

        switch (is.op) {
            case BIN_OPER:
                std::cout << "BIN_OPER    " << getBinOper(is.v1);
                break;

            case JMP:
            case JMPF:
            case JMPT:
                std::cout << getOperator(is.op) << "    -> " << is.v1;
                break;

            case CALL:
                std::cout << "CALL        argc=" << is.v1;
                break;

            case LOAD_GVAR:
            case STORE_GVAR:
            case LOAD_SVAR:
            case STORE_SVAR:
                std::cout << getOperator(is.op) << "    " << is.v1;
                break;

            case LOAD_SCST:
                std::cout << "LOAD_SCST   " << is.v1;
                if (is.v1 >= 0 && is.v1 < (int)fn->constants.size()) {
                    std::cout << "    ; ";
                    showConstValue(fn->constants[is.v1]);
                }
                break;

            case LOAD_GCST:
                std::cout << "LOAD_GCST   " << is.v1;
                if (module && is.v1 >= 0 && is.v1 < (int)module->globalConstPool.size()) {
                    std::cout << "    ; ";
                    showConstValue(module->globalConstPool[is.v1]);
                }
                break;

            case IMM:
                std::cout << "IMM         " << is.v1;
                break;

            case LOAD_MODULE_MEMBER:
                std::cout << "LOAD_MODULE_MEMBER   " << is.v1;
                if (is.v1 >= 0 && is.v1 < (int)fn->constants.size()) {
                    std::cout << "    ; ";
                    showConstValue(fn->constants[is.v1]);
                }
                break;

            case NOP:        case POP:
            case BNOT:       case BBNOT:      case BNEG:
            case RET:        case NEW:        case DUP:
            case MEM_GET:    case MEM_SET:
            case EL_GET:     case EL_SET:
            case NEW_ARR:
            case LOAD_TRUE:  case LOAD_FALSE: case LOAD_NULL:
                std::cout << getOperator(is.op);
                break;

            default:
                std::cout << getOperator(is.op)
                          << "    " << is.v1 << "  " << is.v2;
                break;
        }
        std::cout << "\n";
    }
}

void Dis::disFunction(Function* func) {
    std::cout << "=== Function '" << func->name << "'";
    if (func->isNative) std::cout << " (native)";
    std::cout << " ===\n";
    showConstPool(func);
    std::cout << "  code (" << func->ins.size() << "):\n";
    showInstruation(func);
    std::cout << "\n";
}

void Dis::disModule() {
    std::cout << "=== Module '" << (module ? module->moduleName : "?") << "' ===\n";
    if (!module) return;

    std::cout << "  globalConstPool (" << module->globalConstPool.size() << "):\n";
    for (int i = 0; i < (int)module->globalConstPool.size(); ++i) {
        std::cout << "    " << i << ": ";
        showConstValue(module->globalConstPool[i]);
        std::cout << "\n";
    }

    std::cout << "  globalVars (" << module->globalVars.size() << "):\n";
    for (int i = 0; i < (int)module->globalVars.size(); ++i) {
        std::cout << "    " << i << ": ";
        showConstValue(module->globalVars[i]);
        std::cout << "\n";
    }
    std::cout << "\n";
}

void Dis::disGlobalConstPool() {
    if (!module) return;
    std::cout << "=== GlobalConstPool of '" << module->moduleName << "' ("
              << module->globalConstPool.size() << ") ===\n";
    for (int i = 0; i < (int)module->globalConstPool.size(); ++i) {
        std::cout << "  " << i << ": ";
        showConstValue(module->globalConstPool[i]);
        std::cout << "\n";
    }
    std::cout << "\n";
}

void Dis::disClass(ObjClass* cls) {
    if (!cls) return;
    std::cout << "=== Class '" << cls->name << "'";
    if (cls->super) std::cout << " : " << cls->super->name;
    std::cout << " ===\n";

    std::cout << "  fields (" << cls->fields.size() << "):";
    for (auto& f : cls->fields) std::cout << " " << f;
    std::cout << "\n";

    std::cout << "  methods (" << cls->methods.size() << "):\n\n";
    for (auto& [name, mv] : cls->methods) {
        if (mv.kind == MicaValue::OBJ && mv.obj->tp == Obj::FUNCTION)
            disFunction((Function*)mv.obj);
    }
}

void Dis::disAll() {
    if (!module) return;

    disModule();

    std::cout << "=== All Functions / Classes ===\n\n";
    for (auto& i : module->globalConstPool) {
        if (i.kind != MicaValue::OBJ) continue;

        if (i.obj->tp == Obj::FUNCTION) {
            disFunction((Function*)i.obj);
        } else if (i.obj->tp == Obj::USER_DEFING_CLASS) {
            disClass((ObjClass*)i.obj);
        }
    }
}