#include "headers/datetime.h"

#include <catch2/catch_test_macros.hpp>
#include <chrono>

using namespace std::chrono;

namespace {
sys_seconds secondsAgo(seconds delta) {
    return floor<std::chrono::seconds>(system_clock::now() - delta);
}
}  // namespace

// Названия TEST_CASE — латиницей: ctest/catch_discover_tests передаёт их
// подпроцессу через argv командной строки, а Windows-консоль ломает кириллицу
// в аргументах (реально поймано — см. журнал решений в ROADMAP). Комментарии
// и проверяемые строки остаются русскими, они через argv не проходят.
TEST_CASE("formatRelativeDate: just now", "[datetime]") {
    CHECK(formatRelativeDate(secondsAgo(seconds{0})) == "только что");
    CHECK(formatRelativeDate(secondsAgo(seconds{59})) == "только что");
}

TEST_CASE("formatRelativeDate: N minutes ago", "[datetime]") {
    CHECK(formatRelativeDate(secondsAgo(minutes{1})) == "1 минут назад");
    CHECK(formatRelativeDate(secondsAgo(minutes{45})) == "45 минут назад");
}

TEST_CASE("formatRelativeDate: N hours ago, same calendar day", "[datetime]") {
    auto now = system_clock::now();
    auto sinceMidnight = now - floor<days>(now);
    if (sinceMidnight < hours{2}) {
        return;  // тест пограничен с полуночью (UTC) — см. известное ограничение
                 // "даты в UTC, не в локальном поясе" в ROADMAP; не флейкуем тест
    }
    std::string result = formatRelativeDate(secondsAgo(hours{1} + minutes{30}));
    CHECK(result.find("часов назад") != std::string::npos);
}

TEST_CASE("formatRelativeDate: yesterday", "[datetime]") {
    auto todayMidnight = floor<days>(system_clock::now());
    auto yesterdayEvening = todayMidnight - hours{1};
    CHECK(formatRelativeDate(floor<std::chrono::seconds>(yesterdayEvening)) == "вчера");
}

TEST_CASE("dateToString/stringToDate: round-trip", "[datetime]") {
    sys_seconds original = floor<std::chrono::seconds>(sys_days{2026y / September / 9});
    std::string formatted = dateToString(original);
    sys_seconds parsed = stringToDate(formatted);
    CHECK(parsed == original);
}

TEST_CASE("formatRelativeDate: this year's date omits the year", "[datetime]") {
    auto now = system_clock::now();
    year_month_day today{floor<days>(now)};
    // Полгода назад — но всё ещё в этом же году, иначе тест ломается в январе.
    // Берём фиксированную дату 1 января этого года — гарантированно прошлое
    // и гарантированно тот же год (кроме 1 января, но тогда это уже "вчера"
    // или "сегодня", что проверяется отдельными тестами выше).
    if (unsigned{today.month()} == 1 && unsigned{today.day()} <= 2) {
        return;  // см. комментарий выше — на границе года тест неприменим
    }
    sys_days januaryFirst = sys_days{today.year() / January / 1};
    std::string result = formatRelativeDate(floor<std::chrono::seconds>(januaryFirst));
    CHECK(result.find(std::to_string(int(today.year()))) == std::string::npos);
    CHECK(result.find("января") != std::string::npos);
}

TEST_CASE("formatRelativeDate: last year's date includes the year", "[datetime]") {
    auto now = system_clock::now();
    year_month_day today{floor<days>(now)};
    year lastYear = today.year() - years{1};
    sys_days date = sys_days{lastYear / January / 1};
    std::string result = formatRelativeDate(floor<std::chrono::seconds>(date));
    CHECK(result.find(std::to_string(int(lastYear))) != std::string::npos);
}

TEST_CASE("formatDaySeparator: today and yesterday", "[datetime]") {
    CHECK(formatDaySeparator(secondsAgo(seconds{0})) == "Сегодня");

    auto todayMidnight = floor<days>(system_clock::now());
    auto yesterdayEvening = todayMidnight - hours{1};
    CHECK(formatDaySeparator(floor<std::chrono::seconds>(yesterdayEvening)) == "Вчера");
}

TEST_CASE("formatDaySeparator: older date shows day and month, no time", "[datetime]") {
    auto now = system_clock::now();
    year_month_day today{floor<days>(now)};
    if (unsigned{today.month()} == 1 && unsigned{today.day()} <= 2) {
        return;  // см. аналогичный тест formatRelativeDate выше
    }
    sys_days januaryFirst = sys_days{today.year() / January / 1};
    std::string result = formatDaySeparator(floor<std::chrono::seconds>(januaryFirst));
    CHECK(result == "1 января");
}
