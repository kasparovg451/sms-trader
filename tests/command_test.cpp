#include "headers/command.h"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// Мигрировано с ручного assert (см. ROADMAP, Этап 2, "переезд на Catch2") —
// поведение то же самое, что было в исходном command_module_test.cpp.
// Название TEST_CASE — латиницей: ctest/catch_discover_tests передаёт его
// подпроцессу через argv, а Windows-консоль ломает там кириллицу (см. журнал
// решений в ROADMAP).
TEST_CASE("printCommandList: prints commands from a JSON file", "[command]") {
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

    CHECK(output.str() == "Command: test || Value: checks command output\n");
}
