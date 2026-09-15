#define CROW_ENABLE_SSL
#include <crow.h>
#include <fstream>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_set>

#include "headers/datetime.h"
#include "headers/istorage.h"
#include "headers/message.h"
#include "headers/password_hash.h"
#include "headers/postgres_storage.h"
#include "headers/search.h"
#include "headers/user_storage.h"

using json = nlohmann::json;

// Живые WebSocket-подключения. Пишем/читаем только под wsMutex — сюда заходят
// и обработчики Crow (каждый в своём потоке, multithreaded()), и broadcastNewMessage
// из обработчика POST /messages.
std::mutex wsMutex;
std::unordered_set<crow::websocket::connection*> wsConnections;

void broadcastNewMessage(const json& payload) {
    std::string data = payload.dump();
    std::lock_guard<std::mutex> lock(wsMutex);
    for (crow::websocket::connection* connection : wsConnections) {
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

    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)
    ([&users](const crow::request& req) {
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
        if (password.size() < 8) {
            return jsonResponse(400, {{"error", "password must be at least 8 characters"}});
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
        json body;
        try {
            body = json::parse(req.body);
        } catch (const json::parse_error&) {
            return jsonResponse(400, {{"error", "invalid JSON"}});
        }

        std::string username = body.value("username", "");
        std::string password = body.value("password", "");

        std::optional<User> user = users.findByUsername(username);
        if (!user || !verifyPassword(password, user->passwordHash)) {
            return jsonResponse(401, {{"error", "invalid username or password"}});
        }

        std::string token = createToken(user->id, user->username, jwtSecret);
        return jsonResponse(200, {{"token", token}});
    });

    CROW_ROUTE(app, "/messages").methods(crow::HTTPMethod::GET)
    ([&storage]() {
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

        // author берём из проверенного токена, а не из тела запроса —
        // иначе кто угодно мог бы прислать чужое имя автора.
        Message message = storage.insert(*username, text);
        json payload = messageToJson(message);
        broadcastNewMessage(payload);
        return jsonResponse(201, payload);
    });

    CROW_ROUTE(app, "/ws")
        .websocket(&app)
        .onopen([](crow::websocket::connection& connection) {
            std::lock_guard<std::mutex> lock(wsMutex);
            wsConnections.insert(&connection);
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
    ([&storage](const crow::request& req) {
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

    std::cout << "HTTP API listening on port 8080 (HTTPS)\n";
    app.port(8080).ssl_file("certs/server.crt", "certs/server.key").multithreaded().run();
}
