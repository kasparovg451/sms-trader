#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>


using json = nlohmann::json;

struct Message {
    int id;
    std::string author;
    std::string text;
};

std::string cmdFile {"data/commands.json"};
std::string smsFile {"data/messages.json"};

std::vector<Message> messages;

bool isValidMessage(const json& item) {
    return (
        item.is_object() &&
    item.contains("id") &&
    item.contains("author") &&
    item.contains("text") &&
    item["id"].is_number_integer() &&
    item["author"].is_string() &&
    item["text"].is_string()
    );
}

bool isValidCommand(const json& item) {
    return (
        item.is_object() &&
        item.contains("command") &&
        item.contains("value") &&
        item["command"].is_string() &&
        item["value"].is_string()
    );
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

void printMessages(const std::vector<Message>& messageList) {
    for (const Message& message : messageList) {
        std::cout << message.id << " || " << message.author << ": " << message.text << '\n';
    }
}

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

void searchMessage(const std::string& text_part){
    
}

int main() {
    loadFile(smsFile);

    std::string command;

    printCommandList(cmdFile);

    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, command);

        if (command == "add") {
            addMessage(messages);
            if (saveFile(smsFile, messages)) {
                std::cout << "Messages saved successfully\n";
            }
        } else if (command == "list") {
            printMessages(messages);
        } else if (command == "exit") {
            break;
        } else if (command == "help") {
            printCommandList(cmdFile);
        } else if (command == "count") {
            std::cout << "Count of messages is " << messages.size() << std::endl;
        } else {
            std::cout << "Unknown command: " << command << '\n';
        }
    }

}
