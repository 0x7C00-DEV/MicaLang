//
// Created by Lenovo on 2026/9/26.
//
#include "../include/loader.h"

#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

// =============================================================================
//  MICA module binary format, version 1 (little-endian)
// =============================================================================
//
//  文件布局：
//    [4]  magic      = "MICA" (0x4D494341)
//    [4]  version    = 1
//    [str] moduleName
//
//  类段：
//    [4] classCount
//      per class:
//        [str] name
//        [4]  superIdx          (-1 = 无父类；否则为 classes 下标)
//        [4]  fieldCount
//             per field:  [str] name
//        [4]  methodCount
//             per method: [str] name, [4] funcIdx (0xFFFFFFFF = 未解析)
//
//  函数段：
//    [4] funcCount
//      per function:
//        [str] name
//        [1]  isNative
//        [4]  insCount
//             per ins:   [4] int32
//        [4]  constCount
//             per const: tagged value
//
//  全局常量池：
//    [4] constCount
//         per const: tagged value
//
//  全局变量段：
//    [4] gvarCount
//         per gvar: tagged value
//
//  导出段：
//    [4] exportCount
//         per export: [str] name
//
//  tagged value 见 include/loader.h 的 mica_format::Tag。
// =============================================================================

using namespace mica_format;

namespace {

[[noreturn]] void fail(const std::string& msg) {
    std::cerr << "ProgramLoader: " << msg << std::endl;
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
        in.read(s.data(), static_cast<std::streamsize>(n));
        if (!in) fail("unexpected end of file while reading string");
    }
    return s;
}

std::string readHeader(std::ifstream& in, const std::string& path) {
    uint32_t magic   = readBin<uint32_t>(in);
    uint32_t version = readBin<uint32_t>(in);
    if (magic != MAGIC)
        fail("'" + path + "' is not a MICA module");
    if (version != VERSION)
        fail("unsupported MICA module version " + std::to_string(version));
    return readString(in);
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
            if (idx >= funcs.size()) fail("function ref out of range");
            return MicaValue::Object(funcs[idx]);
        }
        case TAG_CLASS_REF: {
            uint32_t idx = readBin<uint32_t>(in);
            if (idx >= classes.size()) fail("class ref out of range");
            return MicaValue::Object(classes[idx]);
        }
    }
    fail("unknown value tag " + std::to_string(static_cast<int>(tag)));
}


struct ClassMeta {
    std::string name;
    int32_t superIdx = -1;
    std::vector<std::string> fields;
    std::vector<std::pair<std::string, uint32_t>> methods;
};

ClassMeta readClassMeta(std::ifstream& in) {
    ClassMeta m;
    m.name     = readString(in);
    m.superIdx = readBin<int32_t>(in);

    uint32_t fc = readBin<uint32_t>(in);
    m.fields.reserve(fc);
    for (uint32_t i = 0; i < fc; ++i)
        m.fields.push_back(readString(in));

    uint32_t mc = readBin<uint32_t>(in);
    m.methods.reserve(mc);
    for (uint32_t i = 0; i < mc; ++i) {
        std::string n = readString(in);
        uint32_t    ix = readBin<uint32_t>(in);
        m.methods.emplace_back(std::move(n), ix);
    }
    return m;
}


void readFunctionInto(Function* f, std::ifstream& in,
                      const std::vector<Function*>& funcs,
                      const std::vector<ObjClass*>& classes) {
    f->name     = readString(in);
    f->isNative = (readBin<uint8_t>(in) != 0);

    uint32_t ic = readBin<uint32_t>(in);
    f->ins.resize(ic);
    for (uint32_t i = 0; i < ic; ++i)
        f->ins[i] = static_cast<int>(readBin<int32_t>(in));

    uint32_t cc = readBin<uint32_t>(in);
    f->constants.reserve(cc);
    for (uint32_t i = 0; i < cc; ++i)
        f->constants.push_back(readValue(in, funcs, classes));
}

}

ProgramLoader::ProgramLoader(std::string p) : path(std::move(p)) {}

Program* ProgramLoader::getData() {
    std::ifstream in(path, std::ios::binary);
    if (!in) fail("cannot open '" + path + "' for reading");

    readHeader(in, path);

    uint32_t classCount = readBin<uint32_t>(in);
    std::vector<ClassMeta> classMeta(classCount);
    for (uint32_t i = 0; i < classCount; ++i)
        classMeta[i] = readClassMeta(in);

    std::vector<ObjClass*> classes(classCount, nullptr);
    for (uint32_t i = 0; i < classCount; ++i) {
        classes[i] = new ObjClass(classMeta[i].name);
        classes[i]->fields = classMeta[i].fields;
    }

    uint32_t funcCount = readBin<uint32_t>(in);
    std::vector<Function*> funcs(funcCount, nullptr);
    for (uint32_t i = 0; i < funcCount; ++i) funcs[i] = new Function;

    for (uint32_t i = 0; i < funcCount; ++i)
        readFunctionInto(funcs[i], in, funcs, classes);

    for (uint32_t i = 0; i < classCount; ++i) {
        auto& meta = classMeta[i];
        auto* c    = classes[i];
        if (meta.superIdx >= 0) {
            if (static_cast<uint32_t>(meta.superIdx) >= classCount)
                fail("class super index out of range");
            c->super = classes[meta.superIdx];
        }
        for (auto& [mname, idx] : meta.methods) {
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

ModuleSymbol* ProgramLoader::getModuleTag(std::string reName) {
    Program* prog = getData();

    auto* sym = new ModuleSymbol(path, reName);

    for (auto* f : prog->funcs) {
        if (!f) continue;
        auto* ft = new FunctionType(new BaseType(BaseType::MVOID), {});
        sym->funcs[f->name] = new FunctionSymbol(f->name, ft, false);
    }

    for (auto& v : prog->constPools) {
        if (v.kind != MicaValue::OBJ) continue;
        if (v.obj->tp != Obj::USER_DEFING_CLASS) continue;
        auto* c  = (ObjClass*)v.obj;
        auto* cs = new ClassSymbol(c->name,
                                   nullptr,
                                   {},
                                   "",
                                   0,
                                   {},
                                   {});
        sym->cls[c->name] = cs;
    }

    return sym;
}