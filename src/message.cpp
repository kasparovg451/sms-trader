#include <iostream>
#include <string>
#include <vector>

#include "headers/message.h"

std::vector<Message> messages;

void printMessages(const std::vector<Message>& messageList) {
    for (const Message& message : messageList) {
        std::cout << message.id << " || " << message.author << ": " << message.text << '\n';
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
