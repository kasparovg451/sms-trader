#pragma once

#include <libpq-fe.h>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

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
    struct ConnDeleter {
        void operator()(PGconn* connection) const;
    };

    std::mutex connectionMutex_;
    std::unique_ptr<PGconn, ConnDeleter> connection_;
};
