#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "gui/app_config.h"

namespace {

class TempConfig {
public:
    explicit TempConfig(const std::string& contents) {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("smstrader-app-config-" + std::to_string(suffix) + ".json");
        std::ofstream file(path_);
        file << contents;
    }

    ~TempConfig() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

}  // namespace

TEST_CASE("loadAppConfig: missing file uses local HTTPS API") {
    const auto missing = std::filesystem::temp_directory_path() / "smstrader-config-does-not-exist.json";

    const AppConfig config = loadAppConfig(missing);

    REQUIRE(config.apiBaseUrl == "https://localhost:8080");
}

TEST_CASE("loadAppConfig: explicit HTTPS URL is normalized") {
    TempConfig file(R"({"api_base_url":"https://chat.example.com///"})");

    const AppConfig config = loadAppConfig(file.path());

    REQUIRE(config.apiBaseUrl == "https://chat.example.com");
}

TEST_CASE("loadAppConfig: malformed JSON falls back to local API") {
    TempConfig file("{not-json");

    const AppConfig config = loadAppConfig(file.path());

    REQUIRE(config.apiBaseUrl == "https://localhost:8080");
}

TEST_CASE("loadAppConfig: remote plain HTTP is rejected") {
    TempConfig file(R"({"api_base_url":"http://chat.example.com:8080"})");

    const AppConfig config = loadAppConfig(file.path());

    REQUIRE(config.apiBaseUrl == "https://localhost:8080");
}

TEST_CASE("loadAppConfig: loopback plain HTTP is accepted for development") {
    TempConfig localhostFile(R"({"api_base_url":"http://localhost:8080/"})");
    TempConfig loopbackFile(R"({"api_base_url":"http://127.0.0.1:8080/"})");

    REQUIRE(loadAppConfig(localhostFile.path()).apiBaseUrl == "http://localhost:8080");
    REQUIRE(loadAppConfig(loopbackFile.path()).apiBaseUrl == "http://127.0.0.1:8080");
}
