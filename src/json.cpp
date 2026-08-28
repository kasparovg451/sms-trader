#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "headers/json.h"


using json = nlohmann::json;



std::string cmdFile {"data/commands.json"};
std::string smsFile {"data/messages.json"};


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

    if (data.is_array()){
        std::cout << "JSON contains "
                  << data.size()
                  << " messages.\n";
        for (const json& item : data){

            if (!isValidMessage(item)) {
    std::cout << "Invalid message found\n";
    continue;
}
            int id = item["id"].get<int>();
            std::string author = item["author"].get<std::string>();
            std::string text = item["text"].get<std::string>();
            
            Message loadedMessage{id, author, text};
            messages.push_back(loadedMessage);
        }
    } else {
        std::cout << "JSON root is not an array.\n";
    }
}

bool saveFile(
    const std::string& fileName,
    const std::vector<Message>& messageList
){

    json data = json::array();
    for (const Message& message : messageList) {
    json item = {
        {"id", message.id},
        {"author", message.author},
        {"text", message.text}
    };
    data.push_back(item);
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
