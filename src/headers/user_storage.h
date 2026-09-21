#pragma once

#include <optional>
#include <string>

#include "pg_connection.h"

struct User {
    int id;
    std::string username;
    std::string passwordHash;
};

class UserStorage {
public:
    explicit UserStorage(const std::string& connectionString);

    // nullopt, если username уже занят. Полагаемся на UNIQUE constraint в БД,
    // а не на предварительную проверку "есть ли уже такой" — иначе была бы
    // гонка данных между проверкой и вставкой при параллельных регистрациях.
    std::optional<User> createUser(const std::string& username, const std::string& passwordHash);
    std::optional<User> findByUsername(const std::string& username);

private:
    PgConnection connection_;
};
