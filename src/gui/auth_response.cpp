#include "auth_response.h"

#include <nlohmann/json.hpp>

namespace {

AuthResponse transportFailure(const std::string& transportError) {
    if (transportError.empty()) {
        return {};
    }
    return {false, {}, "Ошибка сети: " + transportError};
}

std::string responseError(const std::string& responseBody, const std::string& fallback) {
    try {
        const nlohmann::json response = nlohmann::json::parse(responseBody);
        return response.value("error", fallback);
    } catch (const nlohmann::json::exception&) {
        return "Сервер прислал некорректный ответ";
    }
}

}  // namespace

AuthResponse parseRegistrationResponse(
    int statusCode,
    const std::string& responseBody,
    const std::string& transportError
) {
    if (statusCode == 0 && !transportError.empty()) {
        return transportFailure(transportError);
    }
    if (statusCode == 201) {
        return {true, {}, {}};
    }
    return {false, {}, responseError(responseBody, "неизвестная ошибка регистрации")};
}

AuthResponse parseLoginResponse(
    int statusCode,
    const std::string& responseBody,
    const std::string& transportError
) {
    if (statusCode == 0 && !transportError.empty()) {
        return transportFailure(transportError);
    }

    nlohmann::json response;
    try {
        response = nlohmann::json::parse(responseBody);
    } catch (const nlohmann::json::exception&) {
        return {false, {}, "Сервер прислал некорректный ответ"};
    }

    if (statusCode != 200) {
        return {false, {}, response.value("error", "неизвестная ошибка входа")};
    }

    const std::string token = response.value("token", "");
    if (token.empty()) {
        return {false, {}, "Сервер не вернул токен авторизации"};
    }
    return {true, token, {}};
}
