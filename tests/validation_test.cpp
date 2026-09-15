#include "headers/validation.h"

#include <catch2/catch_test_macros.hpp>

// Названия TEST_CASE — латиницей: ctest/catch_discover_tests передаёт их
// подпроцессу через argv командной строки, а Windows-консоль ломает кириллицу
// в аргументах (реально поймано — см. журнал решений в ROADMAP).

TEST_CASE("isValidMessage: valid message", "[validation]") {
    json item = {{"id", 1}, {"author", "craig"}, {"text", "hi"}, {"date", "2026-09-09T00:00:00Z"}};
    CHECK(isValidMessage(item));
}

TEST_CASE("isValidMessage: not an object", "[validation]") {
    CHECK_FALSE(isValidMessage(json::array()));
    CHECK_FALSE(isValidMessage(json("just a string")));
}

TEST_CASE("isValidMessage: missing field", "[validation]") {
    json missingDate = {{"id", 1}, {"author", "craig"}, {"text", "hi"}};
    CHECK_FALSE(isValidMessage(missingDate));

    json missingAuthor = {{"id", 1}, {"text", "hi"}, {"date", "2026-09-09T00:00:00Z"}};
    CHECK_FALSE(isValidMessage(missingAuthor));
}

TEST_CASE("isValidMessage: wrong field type", "[validation]") {
    // Ровно тот баг, из-за которого появилась валидация в первую очередь
    // (см. журнал решений — assert на item["date"] без проверки типа).
    json wrongDateType = {{"id", 1}, {"author", "craig"}, {"text", "hi"}, {"date", 12345}};
    CHECK_FALSE(isValidMessage(wrongDateType));

    json wrongIdType = {{"id", "1"}, {"author", "craig"}, {"text", "hi"}, {"date", "2026-09-09T00:00:00Z"}};
    CHECK_FALSE(isValidMessage(wrongIdType));
}

TEST_CASE("isValidCommand: valid command", "[validation]") {
    json item = {{"command", "add"}, {"value", "text"}};
    CHECK(isValidCommand(item));
}

TEST_CASE("isValidCommand: missing field or wrong type", "[validation]") {
    CHECK_FALSE(isValidCommand(json{{"command", "add"}}));
    CHECK_FALSE(isValidCommand(json{{"command", 1}, {"value", "text"}}));
}
