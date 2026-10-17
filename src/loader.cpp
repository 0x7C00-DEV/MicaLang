#include "../include/loader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

using namespace mica_format;
namespace fs = std::filesystem;

namespace {

[[noreturn]] void fail(const std::string& msg) {
    std::cerr << "loader: " << msg << std::endl;
    exit(-1);
}

template <typename T>
T readBin(std::ifstream& in) {
    T v;
    in.read(reinterpret_cast<char*>(&v), sizeof(T));
    if (!in) fail("unexpected end of file");
    return v;
}

std::string readString(std::ifstream& in) {
    uint32_t n = readBin<uint32_t>(in);
    if (n > (1u << 24)) fail("string too large");
    std::string s(n, '\0');
    if (n) {
        in.read(s.data(), (std::streamsize)n);
        if (!in) fail("unexpected end of file");
    }
    return s;
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

class TypeContext {
public:
    TypeContext(const std::string& currentModule) : currentModule(currentModule) {}

    ClassSymbol* resolveClass(const std::string& qn) {
        auto it = classes.find(qn);
        if (it != classes.end()) return it->second;
        auto pos = qn.rfind('@');
        std::string mod = pos == std::string::npos ? currentModule : qn.substr(0, pos);
        std::string name = pos == std::string::npos ? qn : qn.substr(pos + 1);
        auto* cs = new ClassSymbol(name, nullptr, {}, mod, nextStubId++, {}, {});
        classes[qn] = cs;
        ownedClasses.push_back(cs);
        return cs;
    }

    InterfaceSymbol* resolveInterface(const std::string& qn) {
        auto it = interfaces.find(qn);
        if (it != interfaces.end()) return it->second;
        auto pos = qn.rfind('@');
        std::string mod = pos == std::string::npos ? currentModule : qn.substr(0, pos);
        std::string name = pos == std::string::npos ? qn : qn.substr(pos + 1);
        auto* is = new InterfaceSymbol(name, {});
        is->moduleName = mod;
        is->interfaceId = nextStubId++;
        interfaces[qn] = is;
        ownedIfaces.push_back(is);
        return is;
    }

    void registerClass(const std::string& qn, ClassSymbol* cs) {
        classes[qn] = cs;
    }
    void registerInterface(const std::string& qn, InterfaceSymbol* is) {
        interfaces[qn] = is;
    }

    std::vector<ClassSymbol*>& allOwnedClasses() { return ownedClasses; }
    std::vector<InterfaceSymbol*>& allOwnedInterfaces() { return ownedIfaces; }

private:
    std::string currentModule;
    std::unordered_map<std::string, ClassSymbol*> classes;
    std::unordered_map<std::string, InterfaceSymbol*> interfaces;
    std::vector<ClassSymbol*> ownedClasses;
    std::vector<InterfaceSymbol*> ownedIfaces;
    int nextStubId = 100000;
};

MType* readType(std::ifstream& in, TypeContext& ctx) {
    uint8_t tag = readBin<uint8_t>(in);
    switch (tag) {
        case TT_BASE: {
            uint8_t id = readBin<uint8_t>(in);
            if (id > (uint8_t)BaseType::MNULL) fail("invalid base type id");
            return new BaseType((BaseType::MicaTypes)id);
        }
        case TT_CLASS: {
            std::string qn = readString(in);
            auto* cs = ctx.resolveClass(qn);
            return new ClassType(cs->name, cs);
        }
        case TT_INTERFACE: {
            std::string qn = readString(in);
            auto* is = ctx.resolveInterface(qn);
            return new InterfaceType(is);
        }
        case TT_ARRAY: {
            auto* elem = readType(in, ctx);
            return new TArrayType(elem);
        }
        case TT_FUNC: {
            auto* ret = readType(in, ctx);
            uint32_t argc = readBin<uint32_t>(in);
            std::vector<MType*> args;
            args.reserve(argc);
            for (uint32_t i = 0; i < argc; ++i)
                args.push_back(readType(in, ctx));
            return new FunctionType(ret, args);
        }
        case TT_TEMPLATE: {
            auto* root = readType(in, ctx);
            uint32_t targc = readBin<uint32_t>(in);
            std::vector<MType*> vars;
            vars.reserve(targc);
            for (uint32_t i = 0; i < targc; ++i)
                vars.push_back(readType(in, ctx));
            return new TTemplateType(root, vars);
        }
        case TT_IMPL: {
            uint32_t n = readBin<uint32_t>(in);
            std::vector<InterfaceSymbol*> ifs;
            ifs.reserve(n);
            for (uint32_t i = 0; i < n; ++i) {
                std::string qn = readString(in);
                ifs.push_back(ctx.resolveInterface(qn));
            }
            return new TImplementsType(ifs);
        }
    }
    fail("unknown type tag");
}


std::string peekMlcModuleName(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    uint32_t magic, version;
    in.read(reinterpret_cast<char*>(&magic), 4);
    in.read(reinterpret_cast<char*>(&version), 4);
    if (!in || magic != MLC_MAGIC || version != VERSION) return "";
    uint32_t n;
    in.read(reinterpret_cast<char*>(&n), 4);
    if (!in || n > (1u << 24)) return "";
    std::string s(n, '\0');
    in.read(s.data(), n);
    if (!in) return "";
    return s;
}

std::string peekSymModuleName(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    uint32_t magic, version;
    in.read(reinterpret_cast<char*>(&magic), 4);
    in.read(reinterpret_cast<char*>(&version), 4);
    if (!in || magic != SYM_MAGIC || version != VERSION) return "";
    uint32_t n;
    in.read(reinterpret_cast<char*>(&n), 4);
    if (!in || n > (1u << 24)) return "";
    std::string s(n, '\0');
    in.read(s.data(), n);
    if (!in) return "";
    return s;
}

MicaValue readValue(std::ifstream& in,
                    const std::vector<Function*>& funcs,
                    const std::vector<ObjClass*>& classes) {
    uint8_t tag = readBin<uint8_t>(in);
    switch (tag) {
        case TAG_NULL:  return MicaValue::Null();
        case TAG_INT:   return MicaValue::Int(readBin<int64_t>(in));
        case TAG_FLOAT: return MicaValue::Float(readBin<double>(in));
        case TAG_BOOL:  return MicaValue::Bool(readBin<uint8_t>(in) != 0);
        case TAG_CHAR:  return MicaValue::Char(readBin<char>(in));
        case TAG_FUNCTION_REF: {
            uint32_t idx = readBin<uint32_t>(in);
            if (idx >= funcs.size()) fail("func ref out of range");
            return MicaValue::Object(funcs[idx]);
        }
        case TAG_CLASS_REF: {
            uint32_t idx = readBin<uint32_t>(in);
            if (idx >= classes.size()) fail("class ref out of range");
            return MicaValue::Object(classes[idx]);
        }
    }
    fail("unknown value tag");
}

} // namespace

// =================== ProgramLoader (.mlc) ===================

ProgramLoader::ProgramLoader(std::string name) : moduleName(std::move(name)) {
    resolvedPath = resolve();
}

std::string ProgramLoader::resolve() {
    { std::ifstream in(moduleName + ".mlc", std::ios::binary); if (in) return moduleName + ".mlc"; }
    { std::ifstream in(moduleName, std::ios::binary); if (in) return moduleName; }

    std::error_code ec;
    for (auto& entry : fs::directory_iterator(".", ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".mlc") continue;
        if (peekMlcModuleName(entry.path().string()) == moduleName)
            return entry.path().string();
    }
    return "";
}

Program* ProgramLoader::getData() {
    if (resolvedPath.empty()) fail("module '" + moduleName + "' not found");
    std::ifstream in(resolvedPath, std::ios::binary);
    if (!in) fail("cannot open '" + resolvedPath + "'");

    uint32_t magic = readBin<uint32_t>(in);
    uint32_t version = readBin<uint32_t>(in);
    if (magic != MLC_MAGIC) fail("not a MICA module");
    if (version != VERSION) fail("unsupported version");
    (void)readString(in);

    struct ClassMeta {
        std::string name;
        int32_t superIdx = -1;
        std::vector<std::string> fields;
        std::vector<std::pair<std::string, uint32_t>> methods;
    };

    uint32_t classCount = readBin<uint32_t>(in);
    std::vector<ClassMeta> classMeta(classCount);
    for (uint32_t i = 0; i < classCount; ++i) {
        auto& cm = classMeta[i];
        cm.name = readString(in);
        cm.superIdx = readBin<int32_t>(in);
        uint32_t fc = readBin<uint32_t>(in);
        cm.fields.reserve(fc);
        for (uint32_t j = 0; j < fc; ++j) cm.fields.push_back(readString(in));
        uint32_t mc = readBin<uint32_t>(in);
        cm.methods.reserve(mc);
        for (uint32_t j = 0; j < mc; ++j) {
            std::string n = readString(in);
            uint32_t ix = readBin<uint32_t>(in);
            cm.methods.emplace_back(std::move(n), ix);
        }
    }

    std::vector<ObjClass*> classes(classCount, nullptr);
    for (uint32_t i = 0; i < classCount; ++i) {
        classes[i] = new ObjClass(classMeta[i].name);
        classes[i]->fields = classMeta[i].fields;
    }

    uint32_t funcCount = readBin<uint32_t>(in);
    std::vector<Function*> funcs(funcCount, nullptr);
    for (uint32_t i = 0; i < funcCount; ++i) funcs[i] = new Function;
    for (uint32_t i = 0; i < funcCount; ++i) {
        auto* f = funcs[i];
        f->name = readString(in);
        f->isNative = (readBin<uint8_t>(in) != 0);
        uint32_t ic = readBin<uint32_t>(in);
        f->ins.resize(ic);
        for (uint32_t j = 0; j < ic; ++j) f->ins[j] = (int)readBin<int32_t>(in);
        uint32_t cc = readBin<uint32_t>(in);
        f->constants.reserve(cc);
        for (uint32_t j = 0; j < cc; ++j)
            f->constants.push_back(readValue(in, funcs, classes));
    }

    for (uint32_t i = 0; i < classCount; ++i) {
        auto& cm = classMeta[i];
        auto* c = classes[i];
        if (cm.superIdx >= 0) {
            if ((uint32_t)cm.superIdx >= classCount) fail("class super index out of range");
            c->super = classes[cm.superIdx];
        }
        for (auto& [mname, idx] : cm.methods) {
            if (idx == 0xFFFFFFFFu) continue;
            if (idx >= funcs.size()) fail("class method funcIdx out of range");
            c->methods[mname] = MicaValue::Object(funcs[idx]);
        }
    }

    auto* prog = new Program;
    prog->funcs = funcs;

    uint32_t constCount = readBin<uint32_t>(in);
    prog->constPools.reserve(constCount);
    for (uint32_t i = 0; i < constCount; ++i)
        prog->constPools.push_back(readValue(in, funcs, classes));

    uint32_t gvarCount = readBin<uint32_t>(in);
    for (uint32_t i = 0; i < gvarCount; ++i)
        (void)readValue(in, funcs, classes);

    uint32_t exportCount = readBin<uint32_t>(in);
    for (uint32_t i = 0; i < exportCount; ++i)
        (void)readString(in);

    return prog;
}

// =================== SymLoader (.sym) ===================

SymLoader::SymLoader(std::string name) : requestedName(std::move(name)) {
    resolvedPath = resolve();
}

std::string SymLoader::resolve() {
    { std::ifstream in(requestedName + ".sym", std::ios::binary); if (in) return requestedName + ".sym"; }
    { std::ifstream in(requestedName + ".mlc", std::ios::binary); if (in) return requestedName + ".mlc"; }
    { std::ifstream in(requestedName, std::ios::binary); if (in) return requestedName; }

    std::error_code ec;
    for (auto& entry : fs::directory_iterator(".", ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        auto p = entry.path().string();
        auto ext = entry.path().extension().string();
        if (ext == ".sym") {
            if (peekSymModuleName(p) == requestedName) return p;
        } else if (ext == ".mlc") {
            if (peekMlcModuleName(p) == requestedName) return p;
        }
    }
    return "";
}

ModuleSymbol* SymLoader::getModuleTag(std::string reName) {
    if (resolvedPath.empty()) return nullptr;

    std::ifstream probe(resolvedPath, std::ios::binary);
    if (!probe) return nullptr;
    uint32_t magic = 0, version = 0;
    probe.read(reinterpret_cast<char*>(&magic), 4);
    probe.read(reinterpret_cast<char*>(&version), 4);
    probe.close();
    if (version != VERSION) return nullptr;

    if (magic == MLC_MAGIC) {
        std::string fileModule = peekMlcModuleName(resolvedPath);
        if (!reName.empty() && fileModule != reName) return nullptr;

        ProgramLoader pl(resolvedPath);
        Program* prog = pl.getData();
        if (!prog) return nullptr;

        auto* sym = new ModuleSymbol(resolvedPath, fileModule);

        for (auto* f : prog->funcs) {
            if (!f) continue;
            auto* ft = new FunctionType(new BaseType(BaseType::MVOID), {});
            sym->funcs[f->name] = new FunctionSymbol(f->name, ft, false);
        }

        std::vector<ObjClass*> classList;
        for (auto& v : prog->constPools) {
            if (v.kind != MicaValue::OBJ) continue;
            if (v.obj->tp != Obj::USER_DEFING_CLASS) continue;
            auto* c = (ObjClass*)v.obj;
            classList.push_back(c);

            auto* cs = new ClassSymbol(c->name, nullptr, {}, fileModule, 0, {}, {});
            for (auto& fname : c->fields) {
                cs->members[fname] = new BaseType(BaseType::MVOID);
                cs->syms[fname] = new VarSymbol(fname, cs->members[fname], false, VarSymbol::ClassMember);
            }
            for (auto& [mname, mv] : c->methods) {
                if (mv.kind != MicaValue::OBJ || mv.obj->tp != Obj::FUNCTION) continue;
                auto* ft = new FunctionType(new BaseType(BaseType::MVOID), {});
                cs->members[mname] = ft;
                cs->syms[mname] = new FunctionSymbol(mname, ft, true);
            }
            sym->cls[c->name] = cs;
        }

        for (auto* c : classList) {
            if (!c->super) continue;
            auto it = sym->cls.find(c->super->name);
            if (it != sym->cls.end())
                sym->cls[c->name]->super = it->second;
        }

        return sym;
    }

    if (magic != SYM_MAGIC) return nullptr;

    std::ifstream in(resolvedPath, std::ios::binary);
    if (!in) return nullptr;

    readBin<uint32_t>(in);  // magic
    readBin<uint32_t>(in);  // version
    std::string fileModule = readString(in);
    if (!reName.empty() && fileModule != reName) return nullptr;

    auto* sym = new ModuleSymbol(resolvedPath, fileModule);
    TypeContext ctx(fileModule);

    struct ClassHeader {
        std::string name;
        std::string superQN;
        std::vector<std::string> implQNs;
    };

    uint32_t ifaceDeclCount = readBin<uint32_t>(in);
    for (uint32_t i = 0; i < ifaceDeclCount; ++i) {
        readString(in);
        readBin<uint8_t>(in);
        uint32_t tp = readBin<uint32_t>(in);
        for (uint32_t j = 0; j < tp; ++j) readString(in);
    }

    uint32_t classCount = readBin<uint32_t>(in);
    std::vector<ClassHeader> classHeaders(classCount);
    std::vector<ClassSymbol*> classSyms(classCount, nullptr);
    for (uint32_t i = 0; i < classCount; ++i) {
        auto& ch = classHeaders[i];
        ch.name = readString(in);
        readBin<uint8_t>(in);
        ch.superQN = readString(in);
        uint32_t implCount = readBin<uint32_t>(in);
        ch.implQNs.resize(implCount);
        for (uint32_t j = 0; j < implCount; ++j)
            ch.implQNs[j] = readString(in);
        uint32_t tp = readBin<uint32_t>(in);
        for (uint32_t j = 0; j < tp; ++j) readString(in);

        auto* cs = new ClassSymbol(ch.name, nullptr, {}, fileModule, (int)i, {}, {});
        classSyms[i] = cs;
        sym->cls[ch.name] = cs;
        ctx.registerClass(fileModule + "@" + ch.name, cs);
    }

    for (uint32_t i = 0; i < classCount; ++i) {
        if (!classHeaders[i].superQN.empty())
            classSyms[i]->super = ctx.resolveClass(classHeaders[i].superQN);
        std::vector<InterfaceSymbol*> impls;
        for (auto& qn : classHeaders[i].implQNs)
            impls.push_back(ctx.resolveInterface(qn));
        classSyms[i]->impl = std::move(impls);
    }

    struct FuncHeader {
        std::string name;
        bool isClassMethod;
    };
    uint32_t funcCount = readBin<uint32_t>(in);
    std::vector<FuncHeader> funcHeaders(funcCount);
    for (uint32_t i = 0; i < funcCount; ++i) {
        funcHeaders[i].name = readString(in);
        readBin<uint8_t>(in);
        funcHeaders[i].isClassMethod = (readBin<uint8_t>(in) != 0);
    }

    uint32_t ifaceContentCount = readBin<uint32_t>(in);
    if (ifaceContentCount != ifaceDeclCount) fail(".sym interface count mismatch");

    uint32_t classContentCount = readBin<uint32_t>(in);
    if (classContentCount != classCount) fail(".sym class count mismatch");
    for (uint32_t i = 0; i < classContentCount; ++i) {
        auto* cs = classSyms[i];

        uint32_t fieldCount = readBin<uint32_t>(in);
        for (uint32_t j = 0; j < fieldCount; ++j) {
            std::string name = readString(in);
            readBin<uint8_t>(in);
            auto* t = readType(in, ctx);
            cs->members[name] = t;
            cs->syms[name] = new VarSymbol(name, t, false, VarSymbol::ClassMember);
        }

        uint32_t methodCount = readBin<uint32_t>(in);
        for (uint32_t j = 0; j < methodCount; ++j) {
            std::string name = readString(in);
            readBin<uint8_t>(in);
            auto* t = readType(in, ctx);
            if (t->baseType != MType::BT_FUNC) fail("method type must be function");
            cs->members[name] = t;
            cs->syms[name] = new FunctionSymbol(name, (FunctionType*)t, true);
        }

        uint32_t nestedCount = readBin<uint32_t>(in);
        for (uint32_t j = 0; j < nestedCount; ++j) {
            std::string name = readString(in);
            readBin<uint8_t>(in);
            auto* is = new InterfaceSymbol(name, {});
            is->moduleName = fileModule + "@" + cs->name;
            is->interfaceId = (int)j;
            uint32_t nm = readBin<uint32_t>(in);
            for (uint32_t k = 0; k < nm; ++k) {
                std::string mn = readString(in);
                readBin<uint8_t>(in);
                auto* ft = (FunctionType*)readType(in, ctx);
                if (ft->baseType != MType::BT_FUNC) fail("interface method must be function");
                is->labels[mn] = new FunctionSymbol(mn, ft, false);
            }
            cs->syms[name] = is;
            cs->members[name] = new InterfaceType(is);
        }
    }

    uint32_t funcContentCount = readBin<uint32_t>(in);
    if (funcContentCount != funcCount) fail(".sym function count mismatch");
    for (uint32_t i = 0; i < funcContentCount; ++i) {
        auto* t = readType(in, ctx);
        if (t->baseType != MType::BT_FUNC) fail("top-level function must be function");
        sym->funcs[funcHeaders[i].name] =
            new FunctionSymbol(funcHeaders[i].name, (FunctionType*)t, funcHeaders[i].isClassMethod);
    }

    return sym;
}