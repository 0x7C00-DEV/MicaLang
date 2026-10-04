//
// Created by Lenovo on 2026/9/26.
//

#ifndef MICALANG_LOADER_H
#define MICALANG_LOADER_H

#include "value.h"
#include "symbol.h"

#include <cstdint>
#include <string>

namespace mica_format {

    constexpr uint32_t MAGIC   = 0x4D494341u;
    constexpr uint32_t VERSION = 1u;

    enum Tag : uint8_t {
        TAG_NULL         = 0,
        TAG_INT          = 1,
        TAG_FLOAT        = 2,
        TAG_BOOL         = 3,
        TAG_CHAR         = 4,
        TAG_FUNCTION_REF = 5,
        TAG_CLASS_REF    = 6,
    };

}

class ProgramLoader {
public:
    explicit ProgramLoader(std::string path);

    Program* getData();

    ModuleSymbol* getModuleTag(std::string reName);

private:
    std::string path;
};

#endif