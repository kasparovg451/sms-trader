#include "headers/pg_connection.h"

#include <stdexcept>

bool PgResult::ok() const {
    ExecStatusType status = PQresultStatus(result_.get());
    return status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK;
}

int PgResult::rowCount() const {
    return PQntuples(result_.get());
}

const char* PgResult::value(int row, int column) const {
    return PQgetvalue(result_.get(), row, column);
}

std::string PgResult::sqlState() const {
    const char* state = PQresultErrorField(result_.get(), PG_DIAG_SQLSTATE);
    return state ? state : "";
}

std::string PgResult::errorMessage() const {
    const char* message = PQresultErrorMessage(result_.get());
    return message ? message : "";
}

PgConnection::PgConnection(std::string connectionString)
    : connectionString_(std::move(connectionString)),
      connection_(PQconnectdb(connectionString_.c_str())) {
    if (PQstatus(connection_.get()) != CONNECTION_OK) {
        // На старте — падаем сразу: без базы сервер бесполезен, и лучше
        // увидеть это в логе запуска, чем на первом запросе.
        throw std::runtime_error(
            std::string("Postgres connection failed: ") + PQerrorMessage(connection_.get())
        );
    }
}

void PgConnection::ensureConnected() {
    if (PQstatus(connection_.get()) == CONNECTION_OK) {
        return;
    }
    // PQreset переоткрывает соединение с теми же параметрами; это блокирующий
    // вызов — под мьютексом остальные запросы подождут, что для одного
    // соединения и так неизбежно.
    PQreset(connection_.get());
    if (PQstatus(connection_.get()) != CONNECTION_OK) {
        throw std::runtime_error(
            std::string("Postgres reconnect failed: ") + PQerrorMessage(connection_.get())
        );
    }
}

PgResult PgConnection::execLocked(const char* sql, const std::vector<std::string>& params) {
    std::vector<const char*> paramValues;
    paramValues.reserve(params.size());
    for (const std::string& param : params) {
        paramValues.push_back(param.c_str());
    }

    return PgResult(PQexecParams(
        connection_.get(), sql,
        static_cast<int>(paramValues.size()),
        nullptr, paramValues.data(), nullptr, nullptr, 0
    ));
}

PgResult PgConnection::exec(const char* sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(mutex_);

    ensureConnected();
    PgResult result = execLocked(sql, params);

    // Запрос мог упасть не из-за SQL, а потому что соединение оборвалось
    // прямо во время него (база перезапустилась). Отличаем по статусу
    // соединения: SQL-ошибка его не портит, обрыв — переводит в BAD.
    if (!result.ok() && PQstatus(connection_.get()) != CONNECTION_OK) {
        ensureConnected();
        // Один повтор. Для INSERT это безопасно: если первый INSERT реально
        // дошёл до базы до обрыва, ответа мы не получили, а транзакция
        // одиночного statement'а либо закоммичена, либо откачена целиком —
        // повтор в худшем случае даст дубль сообщения, не порчу данных.
        // Для 1.0 принимаем; идемпотентные ключи — если станет проблемой.
        result = execLocked(sql, params);
    }

    return result;
}

bool PgConnection::isHealthy() {
    // Через exec(), а не ensureConnected() напрямую: после обрыва со стороны
    // базы PQstatus продолжает показывать CONNECTION_OK, пока libpq не
    // попробует I/O — только реальный запрос выявляет мёртвое соединение,
    // и только exec() умеет после этого переподключиться и повторить.
    try {
        return exec("SELECT 1").ok();
    } catch (const std::runtime_error&) {
        return false;
    }
}
