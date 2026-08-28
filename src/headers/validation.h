#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool isValidMessage(const json& item);

bool isValidCommand(const json& item);
