#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "headers/message.h"
#include "headers/validation.h"


using json = nlohmann::json;

std::vector<Message> messages;

void printMessages(const std::vector<Message>& messageList) {
    for (const Message& message : messageList) {
        std::cout << message.id << " || " << message.author << ": " << message.text << '\n';
    }
}

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
    for (const json& item : data){
        if (!isValidCommand(item)) {
        std::cout << "Invalid command found\n";
        continue;
    }
        std::string command = item["command"].get<std::string>();
        std::string value = item["value"].get<std::string>();

        std::cout << "Command: " << command << " || " << "Value: " << value << std::endl;
    }
}

void addMessage(std::vector<Message>& messageList) {
    int id = static_cast<int>(messageList.size() + 1);
    std::string author;
    std::string text;

    std::cout << "Your name: ";
    std::getline(std::cin, author);

    std::cout << "Your message: ";
    std::getline(std::cin, text);

    Message message{id, author, text};
    messageList.push_back(message);
}
