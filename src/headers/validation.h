#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool isValidMessage(const json& item);

bool isValidCommand(const json& item);

// Лимиты ввода HTTP API (см. ROADMAP, Этап 7, A2). Байты, не символы:
// текст в UTF-8, кириллица — 2 байта на букву, лимит в байтах проще и
// предсказуемее для клиента, чем "символы".
constexpr size_t kMaxMessageTextBytes = 4096;
constexpr size_t kMinUsernameLength = 3;
constexpr size_t kMaxUsernameLength = 32;
constexpr size_t kMinPasswordLength = 8;
// Верхняя граница пароля — защита от DoS: Argon2 с 64 МБ памяти на каждый
// запрос, пароль в мегабайт — это ещё и мегабайт на вход хеш-функции.
constexpr size_t kMaxPasswordLength = 128;

// Строго валидный UTF-8 (без overlong-последовательностей, суррогатов и
// code point'ов выше U+10FFFF). Мусор в кодировке ломает JSON-ответы
// (nlohmann::json::dump бросает на невалидном UTF-8) и вывод в GUI.
bool isValidUtf8(const std::string& text);

// 3–32 символа, только [A-Za-z0-9_]. Латиница сознательно: username — это
// логин/идентификатор, а не отображаемое имя (его добавим отдельным полем).
bool isValidUsername(const std::string& username);
