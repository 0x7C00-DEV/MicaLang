#include "../include/makeError.h"
#include <fstream>
#include <vector>

void makeError(const Register& error) {
    std::ifstream ifs(error.begin.file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    
}