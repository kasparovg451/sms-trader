#pragma once

#include <libpq-fe.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// RAII над PGresult: PQclear в деструкторе, доступ к строкам/ошибкам.
class PgResult {
public:
    explicit PgResult(PGresult* result) : result_(result) {}

    bool ok() const;
    int rowCount() const;
    const char* value(int row, int column) const;
    // Пятисимвольный SQLSTATE ("23505" — нарушение UNIQUE) или пустая строка.
    std::string sqlState() const;
    std::string errorMessage() const;

private:
    struct Deleter {
        void operator()(PGresult* result) const { PQclear(result); }
    };
    std::unique_ptr<PGresult, Deleter> result_;
};

// Одно соединение с Postgres под мьютексом + переподключение.
//
// Зачем: PGconn после рестарта базы (или обрыва сети) переходит в CONNECTION_BAD
// навсегда — без явного PQreset сервер оставался бы мёртвым до ручного
// перезапуска. Здесь: перед запросом проверяем статус, при обрыве — PQreset;
// если запрос упал именно из-за обрыва (не из-за SQL-ошибки) — один повтор.
//
// Один PGconn на процесс под мьютексом — это сознательный потолок масштаба
// (см. ROADMAP, Этап 7, D6); пул соединений — когда упрёмся.
class PgConnection {
public:
    explicit PgConnection(std::string connectionString);

    // Параметризованный запрос ($1, $2, ...). Всегда возвращает результат —
    // проверять ok(); исключение только если соединение не удалось поднять
    // даже после PQreset.
    PgResult exec(const char* sql, const std::vector<std::string>& params = {});

    // Для /health: живо ли соединение (с попыткой поднять, если нет).
    bool isHealthy();

private:
    struct ConnDeleter {
        void operator()(PGconn* connection) const { PQfinish(connection); }
    };

    // Вызывать только под mutex_.
    void ensureConnected();
    PgResult execLocked(const char* sql, const std::vector<std::string>& params);

    std::string connectionString_;
    std::mutex mutex_;
    std::unique_ptr<PGconn, ConnDeleter> connection_;
};
