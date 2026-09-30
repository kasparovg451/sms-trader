#pragma once

#include <filesystem>
#include <string>

struct AppConfig {
    std::string apiBaseUrl;
};

AppConfig loadAppConfig(const std::filesystem::path& path);
