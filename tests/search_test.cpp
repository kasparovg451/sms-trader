#include "headers/search.h"

#include <catch2/catch_test_macros.hpp>

// Названия TEST_CASE — латиницей: ctest/catch_discover_tests передаёт их
// подпроцессу через argv командной строки, а Windows-консоль ломает кириллицу
// в аргументах (реально поймано — см. журнал решений в ROADMAP). Комментарии
// и проверяемые строки остаются русскими, они через argv не проходят.

namespace {
Message makeMessage(int id, const std::string& author, const std::string& text) {
    return Message{id, author, text, std::chrono::sys_seconds{}};
}
}  // namespace

TEST_CASE("searchMessages: exact match is found", "[search]") {
    std::vector<Message> messages{
        makeMessage(1, "craig", "hello world"),
        makeMessage(2, "someone", "unrelated text"),
    };

    auto results = searchMessages("hello", messages);

    REQUIRE(results.size() == 1);
    CHECK(results[0].id == 1);
}

TEST_CASE("searchMessages: typo within threshold is found", "[search]") {
    std::vector<Message> messages{makeMessage(1, "craig", "привет")};

    // "превет" отличается от "привет" на одну букву — по расстоянию
    // Левенштейна это 1/6 ≈ 83% схожести, выше порога 0.7.
    auto results = searchMessages("превет", messages);

    REQUIRE(results.size() == 1);
    CHECK(results[0].id == 1);
}

TEST_CASE("searchMessages: completely different word is not found", "[search]") {
    std::vector<Message> messages{makeMessage(1, "craig", "привет")};

    auto results = searchMessages("автомобиль", messages);

    CHECK(results.empty());
}

TEST_CASE("searchMessages: matches both author and text", "[search]") {
    std::vector<Message> messages{
        makeMessage(1, "alice", "irrelevant"),
        makeMessage(2, "bob", "irrelevant"),
    };

    auto results = searchMessages("alice", messages);

    REQUIRE(results.size() == 1);
    CHECK(results[0].id == 1);
}

TEST_CASE("searchMessages: sorted by descending similarity", "[search]") {
    std::vector<Message> messages{
        // "tests" от "test": расстояние 1, длина 5 -> схожесть 0.8 (проходит порог 0.7)
        makeMessage(1, "author", "tests"),
        // "test" от "test": точное совпадение -> схожесть 1.0
        makeMessage(2, "author", "test"),
    };

    auto results = searchMessages("test", messages);

    REQUIRE(results.size() == 2);
    CHECK(results[0].id == 2);
}

TEST_CASE("searchMessages: caps at 5 results", "[search]") {
    std::vector<Message> messages;
    for (int i = 0; i < 10; ++i) {
        messages.push_back(makeMessage(i, "author", "matching text"));
    }

    auto results = searchMessages("matching text", messages);

    CHECK(results.size() == 5);
}

TEST_CASE("searchMessages: empty query finds nothing", "[search]") {
    std::vector<Message> messages{makeMessage(1, "craig", "hello")};

    auto results = searchMessages("", messages);

    CHECK(results.empty());
}

TEST_CASE("searchMessages: empty message list", "[search]") {
    std::vector<Message> messages;

    auto results = searchMessages("anything", messages);

    CHECK(results.empty());
}
