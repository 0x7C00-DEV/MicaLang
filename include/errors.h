#ifndef ERRORS
#define ERRORS
#include <iostream>
#include "position.h"

const std::string SYNTAX_ERROR = "SyntaxError";
const std::string TYPE_ERROR = "TypeError";
const std::string ZERO_ERROR = "ZeroDivisionError";
const std::string INDEX_ERROR = "IndexError";
const std::string SYSTEM_ERROR = "SystemError";
const std::string NAME_ERROR = "NameError";

class Error {
public:
    std::string errorName;
    std::string errorInfo;
    Position posBegin;
    Position posEnd;
    Error(std::string, std::string, Position, Position);
};

class SyntaxError : public Error {
public:
    SyntaxError(std::string, Position, Position);
};

class TypeError : public Error {
public:
    TypeError(std::string, Position, Position);
};

class ZeroDivisionError : public Error {
public:
    ZeroDivisionError(std::string, Position, Position);
};

class IndexError : public Error {
public:
    IndexError(std::string, Position, Position);
};

class SystemError : public Error {
public:
    SystemError(std::string, Position, Position);
};

class NameError : public Error {
public:
    NameError(std::string, Position, Position);
};

#endif 