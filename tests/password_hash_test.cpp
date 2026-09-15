#include "headers/password_hash.h"

#include <catch2/catch_test_macros.hpp>

// Названия TEST_CASE — латиницей: ctest/catch_discover_tests передаёт их
// подпроцессу через argv командной строки, а Windows-консоль ломает кириллицу
// в аргументах (реально поймано — см. журнал решений в ROADMAP).

TEST_CASE("hashPassword/verifyPassword: correct password verifies", "[password_hash]") {
    std::string hash = hashPassword("correct horse battery staple");
    CHECK(verifyPassword("correct horse battery staple", hash));
}

TEST_CASE("hashPassword/verifyPassword: wrong password fails", "[password_hash]") {
    std::string hash = hashPassword("correct horse battery staple");
    CHECK_FALSE(verifyPassword("wrong password", hash));
}

TEST_CASE("hashPassword: hash does not contain the plaintext password", "[password_hash]") {
    std::string password = "my secret password";
    std::string hash = hashPassword(password);
    CHECK(hash != password);
    CHECK(hash.find(password) == std::string::npos);
}

TEST_CASE("hashPassword: random salt makes identical passwords hash differently", "[password_hash]") {
    // Если бы соль была фиксированной (или отсутствовала), одинаковые пароли
    // давали бы идентичный хеш — находка в БД раскрывала бы, что у двух
    // пользователей одинаковый пароль. Argon2id + RAND_bytes это исключает.
    std::string hashA = hashPassword("same password");
    std::string hashB = hashPassword("same password");
    CHECK(hashA != hashB);

    // Но проверка обоих хешей тем же паролем всё равно должна проходить.
    CHECK(verifyPassword("same password", hashA));
    CHECK(verifyPassword("same password", hashB));
}

TEST_CASE("verifyPassword: malformed hash fails without throwing", "[password_hash]") {
    CHECK_FALSE(verifyPassword("anything", "not a real argon2 hash"));
    CHECK_FALSE(verifyPassword("anything", ""));
}
