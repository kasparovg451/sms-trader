#pragma once

#include <string>

struct AuthResponse {
    bool success{false};
    std::string token;
    std::string error;
};

AuthResponse parseRegistrationResponse(
    int statusCode,
    const std::string& responseBody,
    const std::string& transportError
);

AuthResponse parseLoginResponse(
    int statusCode,
    const std::string& responseBody,
    const std::string& transportError
);
