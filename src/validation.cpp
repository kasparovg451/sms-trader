#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
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

bool isValidUtf8(const std::string& text) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    size_t size = text.size();
    size_t i = 0;

    while (i < size) {
        unsigned char lead = bytes[i];

        if (lead < 0x80) {
            i += 1;
            continue;
        }

        // Определяем длину последовательности по ведущему байту и минимально
        // допустимый code point (overlong-кодирование — запрещено стандартом
        // и классический способ протащить '/' или NUL мимо фильтров).
        size_t length;
        uint32_t codePoint;
        uint32_t minimum;
        if ((lead & 0xE0) == 0xC0) {
            length = 2; codePoint = lead & 0x1F; minimum = 0x80;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3; codePoint = lead & 0x0F; minimum = 0x800;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4; codePoint = lead & 0x07; minimum = 0x10000;
        } else {
            return false;  // одиночный continuation-байт или 0xF8+ — невалидно
        }

        if (i + length > size) {
            return false;  // обрезанная последовательность
        }
        for (size_t k = 1; k < length; ++k) {
            unsigned char continuation = bytes[i + k];
            if ((continuation & 0xC0) != 0x80) {
                return false;
            }
            codePoint = (codePoint << 6) | (continuation & 0x3F);
        }

        if (codePoint < minimum) {
            return false;  // overlong
        }
        if (codePoint >= 0xD800 && codePoint <= 0xDFFF) {
            return false;  // UTF-16 суррогаты в UTF-8 запрещены
        }
        if (codePoint > 0x10FFFF) {
            return false;
        }

        i += length;
    }

    return true;
}

bool isValidUsername(const std::string& username) {
    if (username.size() < kMinUsernameLength || username.size() > kMaxUsernameLength) {
        return false;
    }
    for (unsigned char c : username) {
        bool isLetter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        bool isDigit = (c >= '0' && c <= '9');
        if (!isLetter && !isDigit && c != '_') {
            return false;
        }
    }
    return true;
}