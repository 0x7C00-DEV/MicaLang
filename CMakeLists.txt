cmake_minimum_required(VERSION 4.3)
project(MicaLang)

set(CMAKE_CXX_STANDARD 20)

add_executable(MicaLang main.cpp
        src/token.cpp
        include/lexer.h
        src/lexer.cpp
        include/ast.h
        include/parser.h
        src/parser.cpp
        include/debug.h
        src/debug.cpp
        include/position.h
        src/position.cpp
        include/resolve.h
        src/resolve.cpp
        include/makeError.h
        src/makeError.cpp
        src/asm.cpp)
