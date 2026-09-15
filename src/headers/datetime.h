#pragma once

#include <chrono>
#include <string>

std::string formatRelativeDate(std::chrono::sys_seconds date);

// "Сегодня" / "Вчера" / "29 августа" / "29 августа 2025" — для разделителей
// дат между сообщениями в GUI (в отличие от formatRelativeDate, не показывает
// часы/минуты — только к какому дню относится).
std::string formatDaySeparator(std::chrono::sys_seconds date);

std::string dateToString(std::chrono::sys_seconds date);

std::chrono::sys_seconds stringToDate(std::string date);
