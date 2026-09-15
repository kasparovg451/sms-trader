#pragma once

#include <string>
#include <vector>

#include "message.h"

std::vector<Message> loadFile(const std::string& fileName);

bool saveFile(
    const std::string& fileName,
    const std::vector<Message>& messageList
);
