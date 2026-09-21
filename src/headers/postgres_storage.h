#pragma once

#include <string>

#include "istorage.h"
#include "pg_connection.h"

class PostgresStorage : public IStorage {
public:
    explicit PostgresStorage(const std::string& connectionString);

    std::vector<Message> loadAll() override;
    Message insert(const std::string& author, const std::string& text) override;

    // Для GET /health — соединение с базой живо.
    bool isHealthy();

private:
    PgConnection connection_;
};
