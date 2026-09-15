#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "headers/message.h"

void printMessages(const std::vector<Message>& messageList) {
    for (const Message& message : messageList) {
        std::cout << message.id << " || " << message.author << ": " << message.text << '\n';
    }
}

void MessageStore::add(const std::string& author, const std::string& text) {
    auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    messages_.push_back(Message{nextId_, author, text, now});
    ++nextId_;
}

void MessageStore::addLoaded(Message message) {
    nextId_ = std::max(nextId_, message.id + 1);
    messages_.push_back(std::move(message));
}

const std::vector<Message>& MessageStore::all() const {
    return messages_;
}

size_t MessageStore::count() const {
    return messages_.size();
}
