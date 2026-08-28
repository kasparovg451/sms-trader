#pragma once

#include <string>
#include <vector>

struct Message {
    int id;
    std::string author;
    std::string text;
};

extern std::vector<Message> messages;

void printMessages(const std::vector<Message>& messageList);

void addMessage(std::vector<Message>& messageList);
