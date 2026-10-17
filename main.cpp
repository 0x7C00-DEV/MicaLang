#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "include/repl.h"
#include "include/vm.h"
#include "include/writer.h"

namespace fs = std::filesystem;
using namespace mica_format;


static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

static std::string getExt(const std::string& name) {
    auto pos = name.find_last_of('.');
    if (pos == std::string::npos) return "";
    return name.substr(pos);
}

static std::string stemName(const std::string& name) {
    auto pos = name.find_last_of('.');
    if (pos == std::string::npos) return name;
    return name.substr(0, pos);
}

static std::string extractModuleName(const std::string& code) {
    size_t pos = 0;
    while (pos < code.size()) {
        while (pos < code.size() && std::isspace((unsigned char)code[pos])) ++pos;
        if (pos + 6 <= code.size() && code.compare(pos, 6, "module") == 0) {
            size_t p = pos + 6;
            if (p < code.size() && !std::isalnum((unsigned char)code[p]) && code[p] != '_') {
                while (p < code.size() && std::isspace((unsigned char)code[p])) ++p;
                size_t start = p;
                while (p < code.size() &&
                       (std::isalnum((unsigned char)code[p]) || code[p] == '_')) ++p;
                if (p > start) return code.substr(start, p - start);
            }
        }
        while (pos < code.size() && code[pos] != '\n') ++pos;
        if (pos < code.size()) ++pos;
    }
    return "";
}

static std::string peekModuleName(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    uint32_t magic = 0, version = 0;
    in.read((char*)&magic, 4);
    in.read((char*)&version, 4);
    if (!in || version != VERSION) return "";
    if (magic != MLC_MAGIC && magic != SYM_MAGIC) return "";
    uint32_t n = 0;
    in.read((char*)&n, 4);
    if (!in || n > (1u << 24)) return "";
    std::string s(n, '\0');
    in.read(s.data(), n);
    return in ? s : "";
}

static std::string resolveMlcPath(const std::string& name) {
    { std::ifstream in(name + ".mlc", std::ios::binary); if (in) return name + ".mlc"; }
    { std::ifstream in(name, std::ios::binary); if (in) return name; }

    std::error_code ec;
    for (auto& entry : fs::directory_iterator(".", ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        auto p = entry.path().string();
        if (entry.path().extension() != ".mlc") continue;
        if (peekModuleName(p) == name) return p;
    }
    return "";
}

static std::string resolveSourcePath(const std::string& name) {
    if (getExt(name) == ".mica") {
        std::ifstream in(name);
        return in ? name : "";
    }
    { std::ifstream in(name + ".mica"); if (in) return name + ".mica"; }
    { std::ifstream in(name);           if (in) return name; }
    return "";
}

void exportSymbol(const std::string& code, const std::string& outPath) {
    ErrorCollect ec(nullptr);
    CompileEnvironment env;

    std::string guessed = extractModuleName(code);
    if (guessed.empty()) guessed = stemName(outPath);
    env.moduleName = guessed;

    Compiler compiler(&env, &ec);
    Module* m = compiler.getProgram(code, "<export-symbol>", &ec);
    if (!m) {
        for (auto* e : ec.errors)
            std::cerr << e->errorName << ": " << e->errorInfo << "\n";
        return;
    }

    std::string modName = m->moduleName;
    if (modName.empty()) modName = guessed;

    auto* sym = new ModuleSymbol(outPath, modName);
    for (auto& [name, s] : env.cs.getGlobalScope()->symbols) {
        if (name == "@init") continue;

        if (s->kind == Symbol::SYM_CLASS) {
            sym->cls[name] = (ClassSymbol*)s;
        } else if (s->kind == Symbol::SYM_FUNC) {
            auto* fsym = (FunctionSymbol*)s;
            if (fsym->constPoolIdx < 0) continue;
            auto& v = env.globalConstPool[fsym->constPoolIdx];
            if (v.kind == MicaValue::OBJ && v.obj->tp == Obj::FUNCTION &&
                ((Function*)v.obj)->isNative)
                continue;
            sym->funcs[name] = fsym;
        }
    }
    SymWriter(outPath).write(sym, modName);
    std::cout << "Symbol exported to " << outPath
              << " (module '" << modName << "')\n";
}

void exportCode(const std::string& code, const std::string& outPath) {
    ErrorCollect ec(nullptr);
    CompileEnvironment env;

    std::string guessed = extractModuleName(code);
    if (guessed.empty()) guessed = stemName(outPath);
    env.moduleName = guessed;

    Compiler compiler(&env, &ec);
    Module* m = compiler.getProgram(code, "<export-code>", &ec);
    if (!m) {
        for (auto* e : ec.errors)
            std::cerr << e->errorName << ": " << e->errorInfo << "\n";
        return;
    }

    std::string modName = m->moduleName;
    if (modName.empty()) modName = guessed;

    Program prog;
    std::vector<std::string> exports;
    for (auto& v : m->globalConstPool) {
        if (v.kind == MicaValue::OBJ && v.obj->tp == Obj::FUNCTION)
            prog.funcs.push_back((Function*)v.obj);
        prog.constPools.push_back(v);
    }
    for (auto& [name, s] : env.cs.getGlobalScope()->symbols) {
        if (name == "@init") continue;
        if (s->kind == Symbol::SYM_CLASS || s->kind == Symbol::SYM_FUNC)
            exports.push_back(name);
    }
    ProgramWriter(outPath).write(&prog, modName, exports);
    std::cout << "Code exported to " << outPath
              << " (module '" << modName << "')\n";
}

Module* loadCode(const std::string& path) {
    std::string resolved = resolveMlcPath(path);
    if (resolved.empty()) {
        std::cerr << "Bytecode '" << path << "' not found\n";
        return nullptr;
    }

    ProgramLoader loader(resolved);
    Program* p = loader.getData();
    if (!p) return nullptr;

    std::string modName = peekModuleName(resolved);
    if (modName.empty()) modName = stemName(resolved);

    auto* m = new Module;
    m->moduleName = modName;
    m->globalConstPool = p->constPools;
    for (auto* f : p->funcs) {
        f->module = m;
        m->globalConstPool.push_back(MicaValue::Object(f));
    }
    m->isReady = false;
    return m;
}

static bool isCmd(const std::string& a, const std::string& name) {
    return a == "/" + name || a == "--" + name || a == "-" + name;
}

static void printUsage() {
    std::cout
        << "Usage:\n"
        << "  MicaLang                          enter REPL\n"
        << "  MicaLang /run  <file>             run bytecode (.mlc)\n"
        << "  MicaLang /code <file>             compile source to <stem>.mlc\n"
        << "  MicaLang /sym  <file>             compile source to <stem>.sym\n"
        << "  MicaLang /help                    show this message\n";
}

int main(int argc, char** argv) {
    if (argc == 1) {
        ErrorCollect ec(nullptr);
        Repl repl(&ec);
        repl.run();
        return 0;
    }

    std::string cmd = argv[1];

    if (isCmd(cmd, "help") || isCmd(cmd, "h")) {
        printUsage();
        return 0;
    }

    if (argc < 3) {
        printUsage();
        return 1;
    }

    std::string fileName = argv[2];

    if (isCmd(cmd, "run")) {
        Module* m = loadCode(fileName);
        if (!m) return 1;
        VM vm(m);
        return 0;
    }

    if (isCmd(cmd, "code") || isCmd(cmd, "sym")) {
        std::string srcPath = resolveSourcePath(fileName);
        if (srcPath.empty()) {
            std::cerr << "Source '" << fileName << "' not found\n";
            return 1;
        }
        std::string code = readFile(srcPath);
        if (code.empty()) {
            std::cerr << "Source '" << srcPath << "' is empty\n";
            return 1;
        }

        std::string stem = stemName(srcPath);
        if (isCmd(cmd, "sym")) {
            exportSymbol(code, stem + ".sym");
        } else {
            exportCode(code, stem + ".mlc");
        }
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    printUsage();
    return 1;
}