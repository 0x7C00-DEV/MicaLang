#include "../include/writer.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace mica_format;

namespace {

[[noreturn]] void fail(const std::string& msg) {
    std::cerr << "writer: " << msg << std::endl;
    exit(-1);
}

template <typename T>
void writeBin(std::ofstream& out, const T& v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
    if (!out) fail("write failed");
}

void writeString(std::ofstream& out, const std::string& s) {
    writeBin<uint32_t>(out, (uint32_t)s.size());
    if (!s.empty()) out.write(s.data(), (std::streamsize)s.size());
    if (!out) fail("write failed");
}

std::string qualifiedSymbol(const Symbol* sym) {
    if (sym->kind == Symbol::SYM_CLASS) {
        auto* cs = (ClassSymbol*)sym;
        return cs->clsModule + "@" + cs->name;
    }
    if (sym->kind == Symbol::SYM_INTERFACE) {
        auto* is = (InterfaceSymbol*)sym;
        return is->moduleName + "@" + is->name;
    }
    return "";
}

void writeType(std::ofstream& out, MType* t) {
    if (!t) fail("null type");
    switch (t->baseType) {
        case MType::BT_BASIC: {
            auto* b = (BaseType*)t;
            writeBin<uint8_t>(out, TT_BASE);
            writeBin<uint8_t>(out, (uint8_t)b->type);
            return;
        }
        case MType::BT_CLASS: {
            auto* c = (ClassType*)t;
            writeBin<uint8_t>(out, TT_CLASS);
            writeString(out, qualifiedSymbol(c->sym));
            return;
        }
        case MType::BT_INTERFACE: {
            auto* i = (InterfaceType*)t;
            writeBin<uint8_t>(out, TT_INTERFACE);
            writeString(out, qualifiedSymbol(i->symbol));
            return;
        }
        case MType::BT_ARRAY: {
            auto* a = (TArrayType*)t;
            writeBin<uint8_t>(out, TT_ARRAY);
            writeType(out, a->elementType);
            return;
        }
        case MType::BT_FUNC: {
            auto* f = (FunctionType*)t;
            writeBin<uint8_t>(out, TT_FUNC);
            writeType(out, f->retType);
            writeBin<uint32_t>(out, (uint32_t)f->argsType.size());
            for (auto* a : f->argsType) writeType(out, a);
            return;
        }
        case MType::BT_TEMPLATE: {
            auto* tt = (TTemplateType*)t;
            writeBin<uint8_t>(out, TT_TEMPLATE);
            writeType(out, tt->rootType);
            writeBin<uint32_t>(out, (uint32_t)tt->vars.size());
            for (auto* v : tt->vars) writeType(out, v);
            return;
        }
        case MType::BT_IMPL: {
            auto* impl = (TImplementsType*)t;
            writeBin<uint8_t>(out, TT_IMPL);
            writeBin<uint32_t>(out, (uint32_t)impl->interfaces.size());
            for (auto* i : impl->interfaces)
                writeString(out, qualifiedSymbol(i));
            return;
        }
        default:
            fail("unsupported type for serialization");
    }
}

void writeInterfaceBody(std::ofstream& out, InterfaceSymbol* is) {
    std::vector<std::pair<std::string, FunctionSymbol*>> methods;
    for (auto& [name, fs] : is->labels) methods.push_back({name, fs});
    std::sort(methods.begin(), methods.end(),
              [](auto& a, auto& b) { return a.first < b.first; });

    writeBin<uint32_t>(out, (uint32_t)methods.size());
    for (auto& [name, fs] : methods) {
        writeString(out, name);
        writeBin<uint8_t>(out, 0);
        writeType(out, fs->type);
    }
}

struct Collected {
    std::vector<Function*> funcs;
    std::vector<ObjClass*> classes;
    std::unordered_map<Function*, uint32_t> funcIdx;
    std::unordered_map<ObjClass*, uint32_t> classIdx;
};

Collected collectProgram(Program* prog) {
    Collected r;
    std::unordered_set<Function*> seenF;
    std::unordered_set<ObjClass*> seenC;
    std::vector<Function*> workF;
    std::vector<ObjClass*> workC;

    auto noteFunc = [&](Function* f) {
        if (f && seenF.insert(f).second) workF.push_back(f);
    };
    auto noteClass = [&](ObjClass* c) {
        if (c && seenC.insert(c).second) workC.push_back(c);
    };
    auto noteValue = [&](const MicaValue& v) {
        if (v.kind != MicaValue::OBJ) return;
        if (v.obj->tp == Obj::FUNCTION) noteFunc((Function*)v.obj);
        else if (v.obj->tp == Obj::USER_DEFING_CLASS) noteClass((ObjClass*)v.obj);
    };

    for (auto* f : prog->funcs) noteFunc(f);
    for (auto& v : prog->constPools) noteValue(v);

    size_t fi = 0, ci = 0;
    while (fi < workF.size() || ci < workC.size()) {
        while (fi < workF.size()) {
            auto* f = workF[fi++];
            for (auto& v : f->constants) noteValue(v);
        }
        while (ci < workC.size()) {
            auto* c = workC[ci++];
            if (c->super) noteClass(c->super);
            for (auto& [name, mv] : c->methods) noteValue(mv);
        }
    }

    for (size_t i = 0; i < workF.size(); ++i) {
        r.funcIdx[workF[i]] = (uint32_t)i;
        r.funcs.push_back(workF[i]);
    }
    for (size_t i = 0; i < workC.size(); ++i) {
        r.classIdx[workC[i]] = (uint32_t)i;
        r.classes.push_back(workC[i]);
    }
    return r;
}

void writeValue(std::ofstream& out, const MicaValue& v, const Collected& col) {
    switch (v.kind) {
        case MicaValue::INT:
            writeBin<uint8_t>(out, TAG_INT);
            writeBin<int64_t>(out, v.i);
            return;
        case MicaValue::FLOAT:
            writeBin<uint8_t>(out, TAG_FLOAT);
            writeBin<double>(out, v.f);
            return;
        case MicaValue::MBOOL:
            writeBin<uint8_t>(out, TAG_BOOL);
            writeBin<uint8_t>(out, v.b ? 1 : 0);
            return;
        case MicaValue::NUL:
            writeBin<uint8_t>(out, TAG_NULL);
            return;
        case MicaValue::CHAR:
            writeBin<uint8_t>(out, TAG_CHAR);
            writeBin<char>(out, v.c);
            return;
        case MicaValue::OBJ: {
            if (!v.obj) fail("null Obj in constant pool");
            if (v.obj->tp == Obj::FUNCTION) {
                auto it = col.funcIdx.find((Function*)v.obj);
                if (it == col.funcIdx.end()) fail("Function not in collected set");
                writeBin<uint8_t>(out, TAG_FUNCTION_REF);
                writeBin<uint32_t>(out, it->second);
                return;
            }
            if (v.obj->tp == Obj::USER_DEFING_CLASS) {
                auto it = col.classIdx.find((ObjClass*)v.obj);
                if (it == col.classIdx.end()) fail("ObjClass not in collected set");
                writeBin<uint8_t>(out, TAG_CLASS_REF);
                writeBin<uint32_t>(out, it->second);
                return;
            }
            writeBin<uint8_t>(out, TAG_NULL);
            return;
        }
    }
    fail("unknown MicaValue kind");
}

void writeMlcClass(std::ofstream& out, ObjClass* c, const Collected& col) {
    writeString(out, c->name);
    int32_t superIdx = -1;
    if (c->super) {
        auto it = col.classIdx.find(c->super);
        if (it != col.classIdx.end()) superIdx = (int32_t)it->second;
    }
    writeBin<int32_t>(out, superIdx);
    writeBin<uint32_t>(out, (uint32_t)c->fields.size());
    for (auto& n : c->fields) writeString(out, n);
    writeBin<uint32_t>(out, (uint32_t)c->methods.size());
    for (auto& [mname, mv] : c->methods) {
        writeString(out, mname);
        uint32_t idx = 0xFFFFFFFFu;
        if (mv.kind == MicaValue::OBJ && mv.obj->tp == Obj::FUNCTION) {
            auto it = col.funcIdx.find((Function*)mv.obj);
            if (it != col.funcIdx.end()) idx = it->second;
        }
        writeBin<uint32_t>(out, idx);
    }
}

void writeMlcFunction(std::ofstream& out, Function* f, const Collected& col) {
    writeString(out, f->name);
    writeBin<uint8_t>(out, f->isNative ? 1 : 0);
    writeBin<uint32_t>(out, (uint32_t)f->ins.size());
    for (int code : f->ins)
        writeBin<int32_t>(out, (int32_t)code);
    writeBin<uint32_t>(out, (uint32_t)f->constants.size());
    for (auto& v : f->constants) writeValue(out, v, col);
}

} // namespace

// =================== ProgramWriter (.mlc) ===================

ProgramWriter::ProgramWriter(std::string p) : path(std::move(p)) {}

void ProgramWriter::write(Program* prog, const std::string& moduleName,
                          const std::vector<std::string>& exports) {
    if (!prog) fail("null Program");
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) fail("cannot open '" + path + "' for writing");

    writeBin<uint32_t>(out, MLC_MAGIC);
    writeBin<uint32_t>(out, VERSION);
    writeString(out, moduleName);

    Collected col = collectProgram(prog);

    writeBin<uint32_t>(out, (uint32_t)col.classes.size());
    for (auto* c : col.classes) writeMlcClass(out, c, col);

    writeBin<uint32_t>(out, (uint32_t)col.funcs.size());
    for (auto* f : col.funcs) writeMlcFunction(out, f, col);

    writeBin<uint32_t>(out, (uint32_t)prog->constPools.size());
    for (auto& v : prog->constPools) writeValue(out, v, col);

    writeBin<uint32_t>(out, 0);

    writeBin<uint32_t>(out, (uint32_t)exports.size());
    for (auto& name : exports) writeString(out, name);

    out.flush();
    if (!out) fail("write failed on flush");
}

// =================== SymWriter (.sym) ===================

SymWriter::SymWriter(std::string p) : path(std::move(p)) {}

void SymWriter::write(ModuleSymbol* sym, const std::string& moduleName) {
    if (!sym) fail("null ModuleSymbol");

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) fail("cannot open '" + path + "' for writing");

    writeBin<uint32_t>(out, SYM_MAGIC);
    writeBin<uint32_t>(out, VERSION);
    writeString(out, moduleName);

    std::vector<ClassSymbol*> classes;
    for (auto& [name, cs] : sym->cls) {
        if (cs) classes.push_back(cs);
    }
    std::sort(classes.begin(), classes.end(),
              [](auto* a, auto* b) { return a->name < b->name; });

    std::vector<FunctionSymbol*> funcs;
    for (auto& [name, fs] : sym->funcs) funcs.push_back(fs);
    std::sort(funcs.begin(), funcs.end(),
              [](auto* a, auto* b) { return a->name < b->name; });

    writeBin<uint32_t>(out, 0);

    writeBin<uint32_t>(out, (uint32_t)classes.size());
    for (auto* cs : classes) {
        writeString(out, cs->name);
        writeBin<uint8_t>(out, 0);  // access
        writeString(out, cs->super ? qualifiedSymbol(cs->super) : std::string());
        writeBin<uint32_t>(out, (uint32_t)cs->impl.size());
        for (auto* i : cs->impl)
            writeString(out, qualifiedSymbol(i));
        writeBin<uint32_t>(out, 0);  // templateParamCount
    }

    writeBin<uint32_t>(out, (uint32_t)funcs.size());
    for (auto* fs : funcs) {
        writeString(out, fs->name);
        writeBin<uint8_t>(out, 0);  // access
        writeBin<uint8_t>(out, fs->isClassMethod ? 1 : 0);
    }

    writeBin<uint32_t>(out, 0);

    writeBin<uint32_t>(out, (uint32_t)classes.size());
    for (auto* cs : classes) {
        std::vector<std::pair<std::string, MType*>> fields, methods;
        std::vector<std::pair<std::string, InterfaceSymbol*>> nested;

        for (auto& [name, mt] : cs->members) {
            auto it = cs->syms.find(name);
            if (it == cs->syms.end() || !it->second) continue;
            switch (it->second->kind) {
                case Symbol::SYM_VAR:
                    fields.push_back({name, mt});
                    break;
                case Symbol::SYM_FUNC:
                    methods.push_back({name, mt});
                    break;
                case Symbol::SYM_INTERFACE:
                    nested.push_back({name, ((InterfaceType*)mt)->symbol});
                    break;
                default:
                    break;
            }
        }

        auto byName = [](auto& a, auto& b) { return a.first < b.first; };
        std::sort(fields.begin(), fields.end(), byName);
        std::sort(methods.begin(), methods.end(), byName);
        std::sort(nested.begin(), nested.end(), byName);

        writeBin<uint32_t>(out, (uint32_t)fields.size());
        for (auto& [name, t] : fields) {
            writeString(out, name);
            writeBin<uint8_t>(out, 0);
            writeType(out, t);
        }

        writeBin<uint32_t>(out, (uint32_t)methods.size());
        for (auto& [name, t] : methods) {
            writeString(out, name);
            writeBin<uint8_t>(out, 0);
            writeType(out, t);
        }

        writeBin<uint32_t>(out, (uint32_t)nested.size());
        for (auto& [name, is] : nested) {
            writeString(out, name);
            writeBin<uint8_t>(out, 0);
            writeInterfaceBody(out, is);
        }
    }

    writeBin<uint32_t>(out, (uint32_t)funcs.size());
    for (auto* fs : funcs) {
        writeType(out, fs->type);
    }

    out.flush();
    if (!out) fail("write failed on flush");
}