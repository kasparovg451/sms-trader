#pragma once

#include <string>
#include <vector>

#include "message.h"

std::vector<Message> searchMessages(const std::string& query, const std::vector<Message>& messages);
