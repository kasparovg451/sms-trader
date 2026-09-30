#pragma once

#include <functional>
#include <string>

enum class AuthIntent {
    Login,
    Register,
};

struct AuthFlowResult {
    bool success{false};
    std::string error;
};

using AuthOperation = std::function<AuthFlowResult()>;

AuthFlowResult runAuthFlow(
    AuthIntent intent,
    const AuthOperation& registerOperation,
    const AuthOperation& loginOperation
);
