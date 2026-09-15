#pragma once

#include <libpq-fe.h>
#include <memory>
#include <mutex>
#include <string>

#include "istorage.h"

// pqxx::connection (C++ обёртка) вызывала конфликт линковки на Windows:
// её DLL экспортирует символы с std::string_view в интерфейсе, и это
// сталкивается с той же инстанциацией шаблона в нашем коде (известная
// проблема STL через границу DLL в MSVC). Используем libpq напрямую —
// чистый C API, эта проблема физически невозможна.
class PostgresStorage : public IStorage {
public:
    explicit PostgresStorage(const std::string& connectionString);

    std::vector<Message> loadAll() override;
    Message insert(const std::string& author, const std::string& text) override;

private:
    struct ConnDeleter {
        void operator()(PGconn* connection) const;
    };

    std::mutex connectionMutex_;
    std::unique_ptr<PGconn, ConnDeleter> connection_;
};
