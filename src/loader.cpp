//
// Created by Lenovo on 2026/9/26.
//
#include "../include/loader.h"

#include <fstream>
#include <iostream>
#include <utility>
#include <vector>



using namespace mica_format;

namespace {

[[noreturn]] void fail(const std::string& msg) {

}

template <typename T>
T readBin(std::ifstream& in) {

}

std::string readString(std::ifstream& in) {

}

std::string readHeader(std::ifstream& in, const std::string& path) {

}


MicaValue readValue(std::ifstream& in,
                    const std::vector<Function*>& funcs,
                    const std::vector<ObjClass*>& classes) {

}


struct ClassMeta {
    std::string name;
    int32_t superIdx = -1;
    std::vector<std::string> fields;
    std::vector<std::pair<std::string, uint32_t>> methods;
};

ClassMeta readClassMeta(std::ifstream& in) {

}


void readFunctionInto(Function* f, std::ifstream& in,
                      const std::vector<Function*>& funcs,
                      const std::vector<ObjClass*>& classes) {

}

}

ProgramLoader::ProgramLoader(std::string p) : path(std::move(p)) {}

Program* ProgramLoader::getData() {

}

ModuleSymbol* ProgramLoader::getModuleTag(std::string reName) {

}