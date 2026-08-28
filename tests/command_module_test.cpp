#include "headers/command.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

int main() {
    const std::filesystem::path fileName{"command_module_test.json"};

    {
        std::ofstream file(fileName);
        file << R"([{"command":"test","value":"checks command output"}])";
    }

    std::ostringstream output;
    std::streambuf* originalOutput = std::cout.rdbuf(output.rdbuf());
    printCommandList(fileName.string());
    std::cout.rdbuf(originalOutput);

    std::filesystem::remove(fileName);

    assert(output.str() == "Command: test || Value: checks command output\n");
}
