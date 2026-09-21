#include "headers/postgres_storage.h"

#include <chrono>
#include <stdexcept>

namespace {

Message rowToMessage(const PgResult& result, int row) {
    return Message{
        std::stoi(result.value(row, 0)),
        result.value(row, 1),
        result.value(row, 2),
        std::chrono::sys_seconds{std::chrono::seconds{std::stoll(result.value(row, 3))}}
    };
}

} // namespace

PostgresStorage::PostgresStorage(const std::string& connectionString)
    : connection_(connectionString) {}

std::vector<Message> PostgresStorage::loadAll() {
    // EXTRACT(EPOCH ...) отдаёт готовое unix-время числом — не нужно парсить
    // текстовый формат даты Postgres (там свой формат с часовым поясом).
    PgResult result = connection_.exec(
        "SELECT id, author, text, EXTRACT(EPOCH FROM created_at)::bigint AS epoch "
        "FROM messages ORDER BY id"
    );

    if (!result.ok()) {
        throw std::runtime_error("Postgres query failed: " + result.errorMessage());
    }

    std::vector<Message> messages;
    int rowCount = result.rowCount();
    messages.reserve(static_cast<size_t>(rowCount));
    for (int i = 0; i < rowCount; ++i) {
        messages.push_back(rowToMessage(result, i));
    }

    return messages;
}

Message PostgresStorage::insert(const std::string& author, const std::string& text) {
    // Параметризованный запрос ($1, $2) — защита от SQL-инъекций.
    // Если бы мы собирали запрос конкатенацией строк, сообщение вида
    // "'; DROP TABLE messages; --" могло бы разрушить базу.
    PgResult result = connection_.exec(
        "INSERT INTO messages (author, text) VALUES ($1, $2) "
        "RETURNING id, author, text, EXTRACT(EPOCH FROM created_at)::bigint AS epoch",
        {author, text}
    );

    if (!result.ok()) {
        throw std::runtime_error("Postgres insert failed: " + result.errorMessage());
    }

    return rowToMessage(result, 0);
}

bool PostgresStorage::isHealthy() {
    return connection_.isHealthy();
}
