#include <catch2/catch_test_macros.hpp>

#include "gui/auth_response.h"

TEST_CASE("parseRegistrationResponse: transport failure stays in the login flow") {
    const AuthResponse result = parseRegistrationResponse(0, {}, "Connection refused");

    REQUIRE_FALSE(result.success);
    REQUIRE(result.error == "Ошибка сети: Connection refused");
}

TEST_CASE("parseRegistrationResponse: successful registration needs no token") {
    const AuthResponse result = parseRegistrationResponse(
        201,
        R"({"id":7,"username":"alice"})",
        {}
    );

    REQUIRE(result.success);
    REQUIRE(result.token.empty());
    REQUIRE(result.error.empty());
}

TEST_CASE("parseRegistrationResponse: server rejection exposes its message") {
    const AuthResponse result = parseRegistrationResponse(
        409,
        R"({"error":"username already taken"})",
        {}
    );

    REQUIRE_FALSE(result.success);
    REQUIRE(result.error == "username already taken");
}

TEST_CASE("parseLoginResponse: malformed JSON is reported without terminating") {
    const AuthResponse result = parseLoginResponse(200, "not-json", {});

    REQUIRE_FALSE(result.success);
    REQUIRE(result.error == "Сервер прислал некорректный ответ");
}

TEST_CASE("parseLoginResponse: successful response requires a token") {
    const AuthResponse missingToken = parseLoginResponse(200, R"({"username":"alice"})", {});
    const AuthResponse valid = parseLoginResponse(200, R"({"token":"signed.jwt.value"})", {});

    REQUIRE_FALSE(missingToken.success);
    REQUIRE(missingToken.error == "Сервер не вернул токен авторизации");
    REQUIRE(valid.success);
    REQUIRE(valid.token == "signed.jwt.value");
}

TEST_CASE("parseLoginResponse: rejected credentials expose server error") {
    const AuthResponse result = parseLoginResponse(
        401,
        R"({"error":"invalid username or password"})",
        {}
    );

    REQUIRE_FALSE(result.success);
    REQUIRE(result.error == "invalid username or password");
}
