#include "../include/makeError.h"
#include <fstream>
#include <vector>

void makeError(const Register& error) {
    std::ifstream ifs(error.begin.file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line)) lines.push_back(line);
    int idx = std::max(0, error.begin.lin - 1);
    if (idx >= (int)lines.size()) return;
    const auto& eline = lines[idx];
    std::cout << eline << "\n";
    for (int i = 1; i < error.begin.col; ++i) std::cout << ' ';
    int len = std::max(1, error.end.col - error.begin.col);
    for (int i = 0; i < len; ++i) std::cout << '^';
    std::cout << '\n';
}