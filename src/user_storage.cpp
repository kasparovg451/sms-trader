#include "headers/user_storage.h"

#include <stdexcept>

namespace {

// SQLSTATE 23505 = unique_violation (нарушение UNIQUE-ограничения).
constexpr const char* kUniqueViolation = "23505";

User rowToUser(const PgResult& result, int row) {
    return User{
        std::stoi(result.value(row, 0)),
        result.value(row, 1),
        result.value(row, 2)
    };
}

} // namespace

UserStorage::UserStorage(const std::string& connectionString)
    : connection_(connectionString) {}

std::optional<User> UserStorage::createUser(const std::string& username, const std::string& passwordHash) {
    PgResult result = connection_.exec(
        "INSERT INTO users (username, password_hash) VALUES ($1, $2) "
        "RETURNING id, username, password_hash",
        {username, passwordHash}
    );

    if (!result.ok()) {
        if (result.sqlState() == kUniqueViolation) {
            // Имя занято — ожидаемый исход, не ошибка сервера. Раньше любая
            // ошибка INSERT трактовалась как "занято" — теперь различаем:
            // всё остальное (база упала и т.п.) — это 500, не 409.
            return std::nullopt;
        }
        throw std::runtime_error("Postgres insert failed: " + result.errorMessage());
    }

    return rowToUser(result, 0);
}

std::optional<User> UserStorage::findByUsername(const std::string& username) {
    PgResult result = connection_.exec(
        "SELECT id, username, password_hash FROM users WHERE username = $1",
        {username}
    );

    if (!result.ok()) {
        throw std::runtime_error("Postgres query failed: " + result.errorMessage());
    }
    if (result.rowCount() == 0) {
        return std::nullopt;
    }

    return rowToUser(result, 0);
}
