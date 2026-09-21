#define CROW_ENABLE_SSL
#include <crow.h>
#include <fstream>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>

#include "headers/datetime.h"
#include "headers/istorage.h"
#include "headers/message.h"
#include "headers/password_hash.h"
#include "headers/postgres_storage.h"
#include "headers/search.h"
#include "headers/user_storage.h"
#include "headers/validation.h"

using json = nlohmann::json;

// Лимит тела HTTP-запроса. У Crow своего лимита нет (проверено по коду) —
// проверяем сами в обработчиках. Настоящий отсекающий лимит — на прокси
// (Caddy request_body), это вторая линия.
constexpr size_t kMaxRequestBodyBytes = 64 * 1024;

// Живые WebSocket-подключения -> имя пользователя (из проверенного JWT).
// Пишем/читаем только под wsMutex — сюда заходят и обработчики Crow (каждый
// в своём потоке, multithreaded()), и broadcastNewMessage из POST /messages.
// Имя пользователя понадобится для приватных диалогов (рассылка только
// участникам) — см. ROADMAP, 7.G1.
std::mutex wsMutex;
std::unordered_map<crow::websocket::connection*, std::string> wsConnections;

void broadcastNewMessage(const json& payload) {
    std::string data = payload.dump();
    std::lock_guard<std::mutex> lock(wsMutex);
    for (auto& [connection, username] : wsConnections) {
        // send_text сама по себе не потокобезопасна между собой на одном connection,
        // но т.к. мы шлём последовательно из одного потока (обработчика POST) — ок.
        connection->send_text(data);
    }
}

json messageToJson(const Message& message) {
    return {
        {"id", message.id},
        {"author", message.author},
        {"text", message.text},
        {"relativeDate", formatRelativeDate(message.date)},
        // Unix-время (секунды) — помимо готовой строки relativeDate, чтобы
        // клиент мог сам группировать сообщения по дням (разделители дат)
        // без пересчёта на сервере под каждый клиентский часовой пояс.
        {"timestamp", message.date.time_since_epoch().count()}
    };
}

crow::response jsonResponse(int code, const json& body) {
    crow::response response{code, body.dump()};
    response.set_header("Content-Type", "application/json");
    return response;
}

std::string createToken(int userId, const std::string& username, const std::string& secret) {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_type("JWS")
        .set_issuer("smstrader")
        .set_payload_claim("userId", jwt::claim(std::to_string(userId)))
        .set_payload_claim("username", jwt::claim(username))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(24))
        .sign(jwt::algorithm::hs256{secret});
}

// nullopt, если токена нет, он просрочен или подпись не сходится.
std::optional<std::string> verifyToken(const std::string& token, const std::string& secret) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret})
            .with_issuer("smstrader");
        verifier.verify(decoded);
        return decoded.get_payload_claim("username").as_string();
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> authenticate(const crow::request& req, const std::string& secret) {
    std::string header = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";

    if (header.size() <= prefix.size() || header.compare(0, prefix.size(), prefix) != 0) {
        return std::nullopt;
    }

    return verifyToken(header.substr(prefix.size()), secret);
}

// Для WebSocket: заголовок Authorization предпочтительнее (наш Qt-клиент его
// ставит), но браузерный WebSocket API заголовки ставить не умеет — для
// будущего веб-клиента оставляем fallback через ?token=. В логи прокси токен
// из query попадёт — ещё одна причина предпочитать заголовок.
std::optional<std::string> authenticateWebSocket(const crow::request& req, const std::string& secret) {
    if (auto username = authenticate(req, secret)) {
        return username;
    }
    const char* token = req.url_params.get("token");
    if (!token) {
        return std::nullopt;
    }
    return verifyToken(token, secret);
}

// 413, если тело больше лимита; nullopt — если всё в порядке.
std::optional<crow::response> rejectIfTooLarge(const crow::request& req) {
    if (req.body.size() > kMaxRequestBodyBytes) {
        return jsonResponse(413, {{"error", "request body too large"}});
    }
    return std::nullopt;
}

int main() {
    std::ifstream configFile("db_config.json");
    if (!configFile.is_open()) {
        std::cerr << "db_config.json not found (see db_config.example.json)\n";
        return 1;
    }

    json config;
    configFile >> config;
    std::string connectionString = config.at("connection_string").get<std::string>();
    std::string jwtSecret = config.at("jwt_secret").get<std::string>();

    PostgresStorage storage(connectionString);
    UserStorage users(connectionString);

    crow::SimpleApp app;

    CROW_ROUTE(app, "/health")
    ([&storage]() {
        // liveness + readiness в одном: если база недоступна — 503, чтобы
        // Docker healthcheck / прокси перестали слать сюда трафик.
        if (!storage.isHealthy()) {
            return jsonResponse(503, {{"status", "degraded"}, {"database", "unavailable"}});
        }
        return jsonResponse(200, {{"status", "ok"}});
    });

    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)
    ([&users](const crow::request& req) {
        if (auto tooLarge = rejectIfTooLarge(req)) {
            return std::move(*tooLarge);
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (const json::parse_error&) {
            return jsonResponse(400, {{"error", "invalid JSON"}});
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");

        if (username.empty() || password.empty()) {
            return jsonResponse(400, {{"error", "username and password are required"}});
        }
        if (!isValidUsername(username)) {
            return jsonResponse(400, {{"error", "username must be 3-32 characters: letters, digits, underscore"}});
        }
        if (password.size() < kMinPasswordLength) {
            return jsonResponse(400, {{"error", "password must be at least 8 characters"}});
        }
        if (password.size() > kMaxPasswordLength) {
            return jsonResponse(400, {{"error", "password must be at most 128 characters"}});
        }

        std::string hash = hashPassword(password);
        std::optional<User> user = users.createUser(username, hash);

        if (!user) {
            return jsonResponse(409, {{"error", "username already taken"}});
        }

        return jsonResponse(201, {{"id", user->id}, {"username", user->username}});
    });

    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
    ([&users, &jwtSecret](const crow::request& req) {
        if (auto tooLarge = rejectIfTooLarge(req)) {
            return std::move(*tooLarge);
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (const json::parse_error&) {
            return jsonResponse(400, {{"error", "invalid JSON"}});
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");

        // Тот же лимит, что при регистрации: иначе /login — открытая дверь для
        // DoS через Argon2 на пароле в мегабайт (verifyPassword тоже его считает).
        if (password.size() > kMaxPasswordLength || !isValidUsername(username)) {
            return jsonResponse(401, {{"error", "invalid username or password"}});
        }

        std::optional<User> user = users.findByUsername(username);
        if (!user || !verifyPassword(password, user->passwordHash)) {
            return jsonResponse(401, {{"error", "invalid username or password"}});
        }

        std::string token = createToken(user->id, user->username, jwtSecret);
        return jsonResponse(200, {{"token", token}});
    });

    // Чтение — тоже только с токеном. Без этого любой аноним читал весь чат
    // (см. ROADMAP, 7.A1 — это был блокер для выхода наружу).
    CROW_ROUTE(app, "/messages").methods(crow::HTTPMethod::GET)
    ([&storage, &jwtSecret](const crow::request& req) {
        if (!authenticate(req, jwtSecret)) {
            return jsonResponse(401, {{"error", "authentication required"}});
        }

        json list = json::array();
        for (const Message& message : storage.loadAll()) {
            list.push_back(messageToJson(message));
        }
        return jsonResponse(200, list);
    });

    CROW_ROUTE(app, "/messages").methods(crow::HTTPMethod::POST)
    ([&storage, &jwtSecret](const crow::request& req) {
        std::optional<std::string> username = authenticate(req, jwtSecret);
        if (!username) {
            return jsonResponse(401, {{"error", "authentication required"}});
        }
        if (auto tooLarge = rejectIfTooLarge(req)) {
            return std::move(*tooLarge);
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (const json::parse_error&) {
            return jsonResponse(400, {{"error", "invalid JSON"}});
        }

        std::string text = body.value("text", "");
        if (text.empty()) {
            return jsonResponse(400, {{"error", "text is required"}});
        }
        if (text.size() > kMaxMessageTextBytes) {
            return jsonResponse(400, {{"error", "text is too long (max 4096 bytes)"}});
        }
        if (!isValidUtf8(text)) {
            return jsonResponse(400, {{"error", "text is not valid UTF-8"}});
        }

        // author берём из проверенного токена, а не из тела запроса —
        // иначе кто угодно мог бы прислать чужое имя автора.
        Message message = storage.insert(*username, text);
        json payload = messageToJson(message);
        broadcastNewMessage(payload);
        return jsonResponse(201, payload);
    });

    CROW_ROUTE(app, "/ws")
        .websocket(&app)
        // onaccept вызывается до апгрейда соединения: здесь единственное место,
        // где ещё есть HTTP-запрос с заголовками. Проверенное имя передаём в
        // onopen через userdata — других способов у Crow нет.
        .onaccept([&jwtSecret](const crow::request& req, void** userdata) -> bool {
            std::optional<std::string> username = authenticateWebSocket(req, jwtSecret);
            if (!username) {
                return false;  // Crow ответит 400 — без токена соединения нет
            }
            *userdata = new std::string(*username);
            return true;
        })
        .onopen([](crow::websocket::connection& connection) {
            // Забираем строку из userdata и владеем ею через map — сырой указатель
            // дальше не живёт, утечки нет даже если onclose не вызовется.
            std::unique_ptr<std::string> username(static_cast<std::string*>(connection.userdata()));
            connection.userdata(nullptr);
            std::lock_guard<std::mutex> lock(wsMutex);
            wsConnections.emplace(&connection, username ? *username : std::string{});
        })
        .onclose([](crow::websocket::connection& connection, const std::string& /*reason*/, uint16_t /*code*/) {
            std::lock_guard<std::mutex> lock(wsMutex);
            wsConnections.erase(&connection);
        })
        .onmessage([](crow::websocket::connection& /*connection*/, const std::string& /*data*/, bool /*isBinary*/) {
            // Клиент нам ничего не присылает — канал односторонний, сервер -> клиенты,
            // используется только для рассылки новых сообщений в реальном времени.
        });

    CROW_ROUTE(app, "/messages/search")
    ([&storage, &jwtSecret](const crow::request& req) {
        if (!authenticate(req, jwtSecret)) {
            return jsonResponse(401, {{"error", "authentication required"}});
        }

        const char* query = req.url_params.get("q");
        if (!query) {
            return jsonResponse(400, {{"error", "missing q parameter"}});
        }

        json list = json::array();
        for (const Message& message : searchMessages(query, storage.loadAll())) {
            list.push_back(messageToJson(message));
        }
        return jsonResponse(200, list);
    });

    // Клиент по /ws ничего не шлёт — любой payload больше пары байт это
    // либо баг клиента, либо попытка забить память сервера. По умолчанию
    // у Crow лимита нет (UINT64_MAX — проверено по коду).
    app.websocket_max_payload(1024);

    std::cout << "HTTP API listening on port 8080 (HTTPS)\n";
    app.port(8080).ssl_file("certs/server.crt", "certs/server.key").multithreaded().run();
}
