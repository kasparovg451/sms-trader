#include "app_config.h"

#include <fstream>
#include <string_view>

#include <nlohmann/json.hpp>

namespace {

constexpr std::string_view kDefaultApiBaseUrl = "https://localhost:8080";

std::string trimTrailingSlashes(std::string url) {
    while (!url.empty() && url.back() == '/') {
        url.pop_back();
    }
    return url;
}

std::string hostFromUrl(const std::string& url, const std::string_view scheme) {
    const std::size_t begin = scheme.size();
    const std::size_t end = url.find_first_of(":/", begin);
    return url.substr(begin, end == std::string::npos ? end : end - begin);
}

bool isAllowedApiUrl(const std::string& url) {
    constexpr std::string_view httpsScheme = "https://";
    constexpr std::string_view httpScheme = "http://";

    if (url.starts_with(httpsScheme)) {
        return !hostFromUrl(url, httpsScheme).empty();
    }
    if (!url.starts_with(httpScheme)) {
        return false;
    }

    const std::string host = hostFromUrl(url, httpScheme);
    return host == "localhost" || host == "127.0.0.1";
}

}  // namespace

AppConfig loadAppConfig(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {std::string(kDefaultApiBaseUrl)};
    }

    try {
        const nlohmann::json data = nlohmann::json::parse(file);
        if (!data.contains("api_base_url") || !data["api_base_url"].is_string()) {
            return {std::string(kDefaultApiBaseUrl)};
        }

        std::string url = trimTrailingSlashes(data["api_base_url"].get<std::string>());
        if (!isAllowedApiUrl(url)) {
            return {std::string(kDefaultApiBaseUrl)};
        }
        return {std::move(url)};
    } catch (const nlohmann::json::exception&) {
        return {std::string(kDefaultApiBaseUrl)};
    }
}
