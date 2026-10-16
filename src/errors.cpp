#include "../include/errors.h"


Error::Error(std::string errorName, std::string errorInfo, Position begin, Position end) {
    this->errorName = errorName;
    this->errorInfo = errorInfo;
    this->posBegin = begin;
    this->posEnd = end;
}

SyntaxError::SyntaxError(std::string info, Position begin, Position end) : Error("SyntaxError", info, begin, end) {

}

TypeError::TypeError(std::string info, Position begin, Position end) : Error("TypeError", info, begin, end) {

}

ZeroDivisionError::ZeroDivisionError(std::string info, Position begin, Position end) : Error("ZeroDivisionError", info, begin, end) {

}

IndexError::IndexError(std::string info, Position begin, Position end) : Error("IndexError", info, begin, end) {

}

SystemError::SystemError(std::string info, Position begin, Position end) : Error("SystemError", info, begin, end) {

}

NameError::NameError(std::string info, Position begin, Position end) : Error("NameError", info, begin, end) {

}
