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

TEST_CASE("isValidUtf8: valid text", "[validation]") {
    CHECK(isValidUtf8(""));
    CHECK(isValidUtf8("plain ascii"));
    CHECK(isValidUtf8("привет, мир"));           // 2-байтовые
    CHECK(isValidUtf8("\xE2\x82\xAC"));          // € — 3 байта
    CHECK(isValidUtf8("\xF0\x9F\x98\x80"));      // 😀 — 4 байта
}

TEST_CASE("isValidUtf8: malformed sequences are rejected", "[validation]") {
    CHECK_FALSE(isValidUtf8("\x80"));                  // одиночный continuation
    CHECK_FALSE(isValidUtf8("\xC3"));                  // обрезано
    CHECK_FALSE(isValidUtf8("\xC3\x28"));              // плохой continuation
    CHECK_FALSE(isValidUtf8("\xC0\xAF"));              // overlong '/'
    CHECK_FALSE(isValidUtf8("\xE0\x80\xAF"));          // overlong, 3 байта
    CHECK_FALSE(isValidUtf8("\xED\xA0\x80"));          // суррогат U+D800
    CHECK_FALSE(isValidUtf8("\xF4\x90\x80\x80"));      // > U+10FFFF
    CHECK_FALSE(isValidUtf8("\xF8\x88\x80\x80\x80"));  // 5-байтовая форма
}

TEST_CASE("isValidUsername: allowed and rejected names", "[validation]") {
    CHECK(isValidUsername("abc"));
    CHECK(isValidUsername("Craig_1987"));
    CHECK(isValidUsername(std::string(32, 'a')));

    CHECK_FALSE(isValidUsername("ab"));                     // короче 3
    CHECK_FALSE(isValidUsername(std::string(33, 'a')));     // длиннее 32
    CHECK_FALSE(isValidUsername("with space"));
    CHECK_FALSE(isValidUsername("кириллица"));
    CHECK_FALSE(isValidUsername("dash-not-allowed"));
    CHECK_FALSE(isValidUsername(""));
}
