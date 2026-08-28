#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "headers/storage.h"
#include "headers/validation.h"

void loadFile(const std::string& fileName) {
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
        std::cout << "JSON root is not an array.\n";
        return;
    }

    std::cout << "JSON contains " << data.size() << " messages.\n";

    for (const json& item : data) {
        if (!isValidMessage(item)) {
            std::cout << "Invalid message found\n";
            continue;
        }

        const int id = item["id"].get<int>();
        const std::string author = item["author"].get<std::string>();
        const std::string text = item["text"].get<std::string>();

        messages.push_back(Message{id, author, text});
    }
}

bool saveFile(
    const std::string& fileName,
    const std::vector<Message>& messageList
) {
    json data = json::array();

    for (const Message& message : messageList) {
        data.push_back({
            {"id", message.id},
            {"author", message.author},
            {"text", message.text}
        });
    }

    std::ofstream file(fileName);

    if (!file.is_open()) {
        std::cout << "File can't be opened for writing\n";
        return false;
    }

    file << data.dump(4);

    if (!file) {
        std::cout << "Error while writing file\n";
        return false;
    }

    return true;
}
