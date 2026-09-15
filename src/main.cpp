#include <iostream>
#include <string>
#include <utility>
#include "headers/command.h"
#include "headers/message.h"
#include "headers/storage.h"
#include "headers/constants.h"
#include "headers/search.h"
#include "headers/datetime.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    MessageStore store;
    for (Message& message : loadFile(messageFile)) {
        store.addLoaded(std::move(message));
    }

    std::string command;

    printCommandList(commandFile);

    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, command);

        if (command == "add") {
            std::string author;
            std::string text;

            std::cout << "Your name: ";
            std::getline(std::cin, author);

            std::cout << "Your message: ";
            std::getline(std::cin, text);

            store.add(author, text);

            if (saveFile(messageFile, store.all())) {
                std::cout << "Messages saved successfully\n";
            }
        } else if (command == "list") {
            printMessages(store.all());
        } else if (command == "exit") {
            break;
        } else if (command == "help") {
            printCommandList(commandFile);
        } else if (command == "count") {
            std::cout << "Count of messages is " << store.count() << std::endl;
        } else if (command == "search") {
            std::string query;
            std::cout << "Search query: ";
            std::getline(std::cin, query);

            auto results = searchMessages(query, store.all());

            std::cout << "Found messages:\n";
            for (const Message& message : results) {
                std::cout << message.id << " || " << message.author << ": "
                           << message.text << " (" << formatRelativeDate(message.date) << ")\n";
            }
        } else {
            std::cout << "Unknown command: " << command << '\n';
        }
    }
}
