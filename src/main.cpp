#include <iostream>
#include <string>

#include "headers/command.h"
#include "headers/message.h"
#include "headers/storage.h"

int main() {
    const std::string commandFile{"data/commands.json"};
    const std::string messageFile{"data/messages.json"};

    loadFile(messageFile);

    std::string command;

    printCommandList(commandFile);

    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, command);

        if (command == "add") {
            addMessage(messages);
            if (saveFile(messageFile, messages)) {
                std::cout << "Messages saved successfully\n";
            }
        } else if (command == "list") {
            printMessages(messages);
        } else if (command == "exit") {
            break;
        } else if (command == "help") {
            printCommandList(commandFile);
        } else if (command == "count") {
            std::cout << "Count of messages is " << messages.size() << std::endl;
        } else {
            std::cout << "Unknown command: " << command << '\n';
        }
    }

}
