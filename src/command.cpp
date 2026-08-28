#include <fstream>
#include <iostream>
#include <string>

#include "headers/command.h"
#include "headers/validation.h"

void printCommandList(const std::string& fileName) {
    std::ifstream file(fileName);

    if (!file.is_open()) {
        std::cout << "File is not found\n";
        return;
    }

    json data;

    try {
        file >> data;
    }
    catch (const json::parse_error& error) {
        std::cout << "JSON reading error: " << error.what() << '\n';
        return;
    }

    if (!data.is_array()) {
        std::cout << "Commands JSON root is not an array\n";
        return;
    }

    for (const json& item : data) {
        if (!isValidCommand(item)) {
            std::cout << "Invalid command found\n";
            continue;
        }

        const std::string command = item["command"].get<std::string>();
        const std::string value = item["value"].get<std::string>();

        std::cout << "Command: " << command << " || Value: " << value << '\n';
    }
}
