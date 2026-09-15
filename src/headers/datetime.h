#pragma once

#include <chrono>
#include <string>

std::string formatRelativeDate(std::chrono::sys_seconds date);

std::string dateToString(std::chrono::sys_seconds date);

std::chrono::sys_seconds stringToDate(std::string date);
