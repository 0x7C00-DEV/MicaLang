//
// Created by Lenovo on 2026/10/3.
//
#include "../include/writer.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>


using namespace mica_format;

namespace {

[[noreturn]] void fail(const std::string& msg) {
    std::cerr << "ProgramWriter: " << msg << std::endl;
    exit(-1);
}

template <typename T>
void writeBin(std::ofstream& out, const T& v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
    if (!out) fail("write failed");
}

void writeString(std::ofstream& out, const std::string& s) {
    writeBin<uint32_t>(out, static_cast<uint32_t>(s.size()));
    if (!s.empty())
        out.write(s.data(), static_cast<std::streamsize>(s.size()));
    if (!out) fail("write failed");
}


struct Collected {
    std::vector<Function*> funcs;
    std::vector<ObjClass*> classes;
    std::unordered_map<Function*, uint32_t> funcIdx;
    std::unordered_map<ObjClass*, uint32_t> classIdx;
};

Collected collect(Program* prog) {
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
        if (v.obj->tp == Obj::FUNCTION)                noteFunc ((Function*)v.obj);
        else if (v.obj->tp == Obj::USER_DEFING_CLASS)  noteClass((ObjClass*)v.obj);
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
        r.funcIdx[workF[i]] = static_cast<uint32_t>(i);
        r.funcs.push_back(workF[i]);
    }
    for (size_t i = 0; i < workC.size(); ++i) {
        r.classIdx[workC[i]] = static_cast<uint32_t>(i);
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
                if (it == col.funcIdx.end())
                    fail("Function not registered in the writer's collected set");
                writeBin<uint8_t>(out, TAG_FUNCTION_REF);
                writeBin<uint32_t>(out, it->second);
                return;
            }
            if (v.obj->tp == Obj::USER_DEFING_CLASS) {
                auto it = col.classIdx.find((ObjClass*)v.obj);
                if (it == col.classIdx.end())
                    fail("ObjClass not registered in the writer's collected set");
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

void writeClass(std::ofstream& out, ObjClass* c, const Collected& col) {
    writeString(out, c->name);

    int32_t superIdx = -1;
    if (c->super) {
        auto it = col.classIdx.find(c->super);
        if (it != col.classIdx.end())
            superIdx = static_cast<int32_t>(it->second);
    }
    writeBin<int32_t>(out, superIdx);

    writeBin<uint32_t>(out, static_cast<uint32_t>(c->fields.size()));
    for (auto& n : c->fields)
        writeString(out, n);

    writeBin<uint32_t>(out, static_cast<uint32_t>(c->methods.size()));
    for (auto& [mname, mv] : c->methods) {
        writeString(out, mname);

        uint32_t idx = 0xFFFFFFFFu; // 未解析
        if (mv.kind == MicaValue::OBJ && mv.obj->tp == Obj::FUNCTION) {
            auto it = col.funcIdx.find((Function*)mv.obj);
            if (it != col.funcIdx.end()) idx = it->second;
        }
        writeBin<uint32_t>(out, idx);
    }
}

void writeFunction(std::ofstream& out, Function* f, const Collected& col) {
    writeString(out, f->name);
    writeBin<uint8_t>(out, f->isNative ? 1 : 0);

    writeBin<uint32_t>(out, static_cast<uint32_t>(f->ins.size()));
    for (int code : f->ins)
        writeBin<int32_t>(out, static_cast<int32_t>(code));

    writeBin<uint32_t>(out, static_cast<uint32_t>(f->constants.size()));
    for (auto& v : f->constants)
        writeValue(out, v, col);
}

void writeHeader(std::ofstream& out, const std::string& moduleName) {
    writeBin<uint32_t>(out, MAGIC);
    writeBin<uint32_t>(out, VERSION);
    writeString(out, moduleName);
}

}

ProgramWriter::ProgramWriter(std::string p) : path(std::move(p)) {}

void ProgramWriter::write(Program* prog,
                          const std::string& moduleName,
                          const std::vector<std::string>& exports) {
    if (!prog) fail("ProgramWriter::write got a null Program");

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) fail("cannot open '" + path + "' for writing");

    writeHeader(out, moduleName);

    Collected col = collect(prog);

    writeBin<uint32_t>(out, static_cast<uint32_t>(col.classes.size()));
    for (auto* c : col.classes)
        writeClass(out, c, col);

    writeBin<uint32_t>(out, static_cast<uint32_t>(col.funcs.size()));
    for (auto* f : col.funcs)
        writeFunction(out, f, col);

    writeBin<uint32_t>(out, static_cast<uint32_t>(prog->constPools.size()));
    for (auto& v : prog->constPools)
        writeValue(out, v, col);

    writeBin<uint32_t>(out, 0);

    writeBin<uint32_t>(out, static_cast<uint32_t>(exports.size()));
    for (auto& name : exports)
        writeString(out, name);

    out.flush();
    if (!out) fail("write failed on flush");
}