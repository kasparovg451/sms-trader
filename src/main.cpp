#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "headers/validation.h"
#include "headers/message.h"
#include "headers/json.h"

using json = nlohmann::json;

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
            if (saveFile(smsFile, static_cast<const std::vector<Message>&>(messages))) {
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
