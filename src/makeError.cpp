#include "../include/makeError.h"
#include <fstream>
#include <vector>

void makeError(const Register& error) {
    std::ifstream ifs(error.begin.file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    std::string eline = lines[error.begin.lin-2];
    std::cout << eline << std::endl;
    for (int i=0; i<error.end.col; ++i)
        if (i>error.begin.col)
            printf("^");
    std::cout << std::endl;
}