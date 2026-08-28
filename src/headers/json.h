#pragma once

#include <string>
#include <vector>
#include "message.h"
#include "headers/validation.h"

extern std::string cmdFile;
extern std::string smsFile;


void loadFile(const std::string& fileName);

bool saveFile(
    const std::string& fileName,
    const std::vector<Message>& messageList
);
