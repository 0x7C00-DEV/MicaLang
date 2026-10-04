//
// Created by Lenovo on 2026/10/3.
//

#ifndef MICALANG_WRITER_H
#define MICALANG_WRITER_H

#include "loader.h"
#include <string>
#include <vector>

/*
 *
 *
 *
 */

class ProgramWriter {
public:
    explicit ProgramWriter(std::string path);
    void write(Program* prog,
               const std::string& moduleName,
               const std::vector<std::string>& exports = {});

private:
    std::string path;
};

#endif //MICALANG_WRITER_H