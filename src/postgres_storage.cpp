#include "headers/postgres_storage.h"

#include <chrono>
#include <stdexcept>

namespace {

// RAII-обёртка над PGresult — без неё утечка при исключении (например,
// если PQgetvalue вернёт не число, а std::stoll бросит).
struct ResultGuard {
    PGresult* result;
    ~ResultGuard() { PQclear(result); }
};

Message rowToMessage(PGresult* result, int row) {
    return Message{
        std::stoi(PQgetvalue(result, row, 0)),
        PQgetvalue(result, row, 1),
        PQgetvalue(result, row, 2),
        std::chrono::sys_seconds{std::chrono::seconds{std::stoll(PQgetvalue(result, row, 3))}}
    };
}

} // namespace

void PostgresStorage::ConnDeleter::operator()(PGconn* connection) const {
    PQfinish(connection);
}

PostgresStorage::PostgresStorage(const std::string& connectionString)
    : connection_(PQconnectdb(connectionString.c_str())) {
    if (PQstatus(connection_.get()) != CONNECTION_OK) {
        throw std::runtime_error(
            std::string("Postgres connection failed: ") + PQerrorMessage(connection_.get())
        );
    }
}

std::vector<Message> PostgresStorage::loadAll() {
    std::lock_guard<std::mutex> lock(connectionMutex_);

    // EXTRACT(EPOCH ...) отдаёт готовое unix-время числом — не нужно парсить
    // текстовый формат даты Postgres (там свой формат с часовым поясом).
    PGresult* raw = PQexec(
        connection_.get(),
        "SELECT id, author, text, EXTRACT(EPOCH FROM created_at)::bigint AS epoch "
        "FROM messages ORDER BY id"
    );
    ResultGuard guard{raw};

    if (PQresultStatus(raw) != PGRES_TUPLES_OK) {
        throw std::runtime_error(std::string("Postgres query failed: ") + PQerrorMessage(connection_.get()));
    }

    std::vector<Message> messages;
    int rowCount = PQntuples(raw);
    for (int i = 0; i < rowCount; ++i) {
        messages.push_back(rowToMessage(raw, i));
    }

    return messages;
}

Message PostgresStorage::insert(const std::string& author, const std::string& text) {
    std::lock_guard<std::mutex> lock(connectionMutex_);

    // Параметризованный запрос ($1, $2) — защита от SQL-инъекций.
    // Если бы мы собирали запрос конкатенацией строк, сообщение вида
    // "'; DROP TABLE messages; --" могло бы разрушить базу.
    const char* paramValues[2] = {author.c_str(), text.c_str()};
    PGresult* raw = PQexecParams(
        connection_.get(),
        "INSERT INTO messages (author, text) VALUES ($1, $2) "
        "RETURNING id, author, text, EXTRACT(EPOCH FROM created_at)::bigint AS epoch",
        2, nullptr, paramValues, nullptr, nullptr, 0
    );
    ResultGuard guard{raw};

    if (PQresultStatus(raw) != PGRES_TUPLES_OK) {
        throw std::runtime_error(std::string("Postgres insert failed: ") + PQerrorMessage(connection_.get()));
    }

    return rowToMessage(raw, 0);
}
