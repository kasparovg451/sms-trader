#include <array>
#include <chrono>
#include <format>
#include <sstream>
#include <string>

#include "headers/datetime.h"

static const std::array<const char*, 12> monthNamesGenitive {
    "января", "февраля", "марта", "апреля", "мая", "июня",
    "июля", "августа", "сентября", "октября", "ноября", "декабря"
};

std::string formatRelativeDate(std::chrono::sys_seconds date) {
    auto now = std::chrono::system_clock::now();

    auto diff = now - date;

    using namespace std::chrono;

    if (diff < minutes{1}) {
        return "только что";
    }

    if (diff < hours{1}) {
        auto minutesPassed = duration_cast<minutes>(diff).count();
        return std::to_string(minutesPassed) + " минут назад";
    }

    auto todayDays = floor<days>(now);
    auto messageDays = floor<days>(date);
    auto dayDiff = todayDays - messageDays;

    if (dayDiff == days{0}) {
        auto hoursPassed = duration_cast<hours>(diff).count();
        return std::to_string(hoursPassed) + " часов назад";
    }

    if (dayDiff == days{1}) {
        return "вчера";
    }

    year_month_day messageYmd{messageDays};
    year_month_day todayYmd{todayDays};

    unsigned monthIndex = static_cast<unsigned>(messageYmd.month()) - 1;
    unsigned dayOfMonth = static_cast<unsigned>(messageYmd.day());

    if (messageYmd.year() == todayYmd.year()) {
        return std::to_string(dayOfMonth) + " "
        + monthNamesGenitive[monthIndex];
    }

    int yearNumber = static_cast<int>(messageYmd.year());

    return std::to_string(dayOfMonth) + " "
    + monthNamesGenitive[monthIndex] + " "
    + std::to_string(yearNumber);
}

std::string dateToString(std::chrono::sys_seconds date) {
    return std::format("{:%Y-%m-%dT%H:%M:%SZ}", date);
}

std::chrono::sys_seconds stringToDate(std::string date) {

    std::istringstream json_date (date);

    std::chrono::sys_seconds real_date;

    json_date >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", real_date);

    return real_date;
}
