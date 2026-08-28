#pragma once

#include <string>
#include <vector>

#include "message.h"

void loadFile(const std::string& fileName);

bool saveFile(
    const std::string& fileName,
    const std::vector<Message>& messageList
);
