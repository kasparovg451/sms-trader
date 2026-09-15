#include <array>
#include <chrono>
#include <format>
#include <sstream>
#include <string>

#include "headers/datetime.h"

using namespace std::chrono;

static const std::array<const char*, 12> monthNamesGenitive {
    "января", "февраля", "марта", "апреля", "мая", "июня",
    "июля", "августа", "сентября", "октября", "ноября", "декабря"
};

// "29 августа" или "29 августа 2025" — общий хвост и для formatRelativeDate
// (дата дальше вчера), и для formatDaySeparator (разделитель дня в GUI).
static std::string formatDayMonth(year_month_day ymd, bool includeYear) {
    unsigned monthIndex = static_cast<unsigned>(ymd.month()) - 1;
    unsigned dayOfMonth = static_cast<unsigned>(ymd.day());

    std::string result = std::to_string(dayOfMonth) + " " + monthNamesGenitive[monthIndex];
    if (includeYear) {
        result += " " + std::to_string(static_cast<int>(ymd.year()));
    }
    return result;
}

std::string formatRelativeDate(std::chrono::sys_seconds date) {
    auto now = std::chrono::system_clock::now();

    auto diff = now - date;

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

    return formatDayMonth(messageYmd, messageYmd.year() != todayYmd.year());
}

std::string formatDaySeparator(std::chrono::sys_seconds date) {
    auto now = std::chrono::system_clock::now();

    auto todayDays = floor<days>(now);
    auto messageDays = floor<days>(date);
    auto dayDiff = todayDays - messageDays;

    if (dayDiff == days{0}) {
        return "Сегодня";
    }
    if (dayDiff == days{1}) {
        return "Вчера";
    }

    year_month_day messageYmd{messageDays};
    year_month_day todayYmd{todayDays};

    return formatDayMonth(messageYmd, messageYmd.year() != todayYmd.year());
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
