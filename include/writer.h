#ifndef MICALANG_WRITER_H
#define MICALANG_WRITER_H

#include "value.h"
#include "symbol.h"
#include "loader.h"

#include <string>
#include <vector>

class ProgramWriter {
public:
    explicit ProgramWriter(std::string path);
    void write(Program* prog,
               const std::string& moduleName,
               const std::vector<std::string>& exports = {});
private:
    std::string path;
};

class SymWriter {
public:
    explicit SymWriter(std::string path);
    void write(ModuleSymbol* sym, const std::string& moduleName);
private:
    std::string path;
};

#endif