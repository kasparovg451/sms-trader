#pragma once

#include <chrono>
#include <string>
#include <vector>

struct Message {
    int id;
    std::string author;
    std::string text;
    std::chrono::sys_seconds date;
};

class MessageStore {
public:
    void add(const std::string& author, const std::string& text);
    void addLoaded(Message message);

    const std::vector<Message>& all() const;
    size_t count() const;

private:
    std::vector<Message> messages_;
    int nextId_ = 1;
};

void printMessages(const std::vector<Message>& messageList);
