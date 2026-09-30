#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "gui/auth_flow.h"

TEST_CASE("runAuthFlow: registration success immediately continues to login") {
    std::vector<std::string> calls;

    const AuthFlowResult result = runAuthFlow(
        AuthIntent::Register,
        [&calls] {
            calls.push_back("register");
            return AuthFlowResult{true, {}};
        },
        [&calls] {
            calls.push_back("login");
            return AuthFlowResult{true, {}};
        }
    );

    REQUIRE(result.success);
    REQUIRE(calls == std::vector<std::string>{"register", "login"});
}

TEST_CASE("runAuthFlow: registration failure does not attempt login") {
    bool loginCalled = false;

    const AuthFlowResult result = runAuthFlow(
        AuthIntent::Register,
        [] { return AuthFlowResult{false, "username already taken"}; },
        [&loginCalled] {
            loginCalled = true;
            return AuthFlowResult{true, {}};
        }
    );

    REQUIRE_FALSE(result.success);
    REQUIRE(result.error == "username already taken");
    REQUIRE_FALSE(loginCalled);
}

TEST_CASE("runAuthFlow: login intent skips registration") {
    bool registrationCalled = false;

    const AuthFlowResult result = runAuthFlow(
        AuthIntent::Login,
        [&registrationCalled] {
            registrationCalled = true;
            return AuthFlowResult{true, {}};
        },
        [] { return AuthFlowResult{true, {}}; }
    );

    REQUIRE(result.success);
    REQUIRE_FALSE(registrationCalled);
}
