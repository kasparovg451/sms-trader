#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "headers/validation.h"


using json = nlohmann::json;


bool isValidMessage(const json& item) {
    return (
        item.is_object() &&
    item.contains("id") &&
    item.contains("author") &&
    item.contains("text") &&
    item.contains("date") &&
    item["id"].is_number_integer() &&
    item["author"].is_string() &&
    item["text"].is_string() &&
    item["date"].is_string()
    );
}

bool isValidCommand(const json& item) {
    return (
        item.is_object() &&
        item.contains("command") &&
        item.contains("value") &&
        item["command"].is_string() &&
        item["value"].is_string()
    );
}