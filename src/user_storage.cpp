#include "headers/user_storage.h"

#include <stdexcept>

namespace {

struct ResultGuard {
    PGresult* result;
    ~ResultGuard() { PQclear(result); }
};

User rowToUser(PGresult* result, int row) {
    return User{
        std::stoi(PQgetvalue(result, row, 0)),
        PQgetvalue(result, row, 1),
        PQgetvalue(result, row, 2)
    };
}

} // namespace

void UserStorage::ConnDeleter::operator()(PGconn* connection) const {
    PQfinish(connection);
}

UserStorage::UserStorage(const std::string& connectionString)
    : connection_(PQconnectdb(connectionString.c_str())) {
    if (PQstatus(connection_.get()) != CONNECTION_OK) {
        throw std::runtime_error(
            std::string("Postgres connection failed: ") + PQerrorMessage(connection_.get())
        );
    }
}

std::optional<User> UserStorage::createUser(const std::string& username, const std::string& passwordHash) {
    std::lock_guard<std::mutex> lock(connectionMutex_);

    const char* paramValues[2] = {username.c_str(), passwordHash.c_str()};
    PGresult* raw = PQexecParams(
        connection_.get(),
        "INSERT INTO users (username, password_hash) VALUES ($1, $2) "
        "RETURNING id, username, password_hash",
        2, nullptr, paramValues, nullptr, nullptr, 0
    );
    ResultGuard guard{raw};

    if (PQresultStatus(raw) != PGRES_TUPLES_OK) {
        // Скорее всего, нарушение UNIQUE-ограничения на username — это
        // ожидаемый исход (имя уже занято), а не ошибка сервера.
        return std::nullopt;
    }

    return rowToUser(raw, 0);
}

std::optional<User> UserStorage::findByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(connectionMutex_);

    const char* paramValues[1] = {username.c_str()};
    PGresult* raw = PQexecParams(
        connection_.get(),
        "SELECT id, username, password_hash FROM users WHERE username = $1",
        1, nullptr, paramValues, nullptr, nullptr, 0
    );
    ResultGuard guard{raw};

    if (PQresultStatus(raw) != PGRES_TUPLES_OK || PQntuples(raw) == 0) {
        return std::nullopt;
    }

    return rowToUser(raw, 0);
}
