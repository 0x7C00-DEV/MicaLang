#ifndef MICALANG_LOADER_H
#define MICALANG_LOADER_H

#include "value.h"
#include "symbol.h"

#include <cstdint>
#include <string>

namespace mica_format {

    constexpr uint32_t MLC_MAGIC = 0x4D494341u;  // "MICA"
    constexpr uint32_t SYM_MAGIC = 0x4D494353u;  // "MICS"
    constexpr uint32_t VERSION   = 1u;
    constexpr uint32_t MAGIC     = MLC_MAGIC;    // 向后兼容

    enum Tag : uint8_t {
        TAG_NULL = 0, TAG_INT = 1, TAG_FLOAT = 2, TAG_BOOL = 3,
        TAG_CHAR = 4, TAG_FUNCTION_REF = 5, TAG_CLASS_REF = 6,
    };

    enum TypeTag : uint8_t {
        TT_BASE = 0, TT_CLASS = 1, TT_INTERFACE = 2,
        TT_ARRAY = 3, TT_FUNC = 4, TT_TEMPLATE = 5, TT_IMPL = 6,
    };

}

class ProgramLoader {
public:
    explicit ProgramLoader(std::string name);
    Program* getData();
private:
    std::string moduleName;
    std::string resolvedPath;
    std::string resolve();
};

class SymLoader {
public:
    explicit SymLoader(std::string name);
    ModuleSymbol* getModuleTag(std::string reName);
private:
    std::string requestedName;
    std::string resolvedPath;
    std::string resolve();
};

#endif