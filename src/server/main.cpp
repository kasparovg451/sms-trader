#include <algorithm>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "headers/datetime.h"
#include "headers/istorage.h"
#include "headers/message.h"
#include "headers/postgres_storage.h"
#include "headers/protocol.h"
#include "headers/search.h"

using asio::ip::tcp;
using ssl_socket = asio::ssl::stream<tcp::socket>;
using json = nlohmann::json;

std::mutex clientsMutex;
std::vector<std::shared_ptr<ssl_socket>> clients;

std::mutex storeMutex;
MessageStore store;

std::unique_ptr<IStorage> storage;

json messageToJson(const Message& message) {
    return {
        {"id", message.id},
        {"author", message.author},
        {"text", message.text},
        {"relativeDate", formatRelativeDate(message.date)}
    };
}

void sendTo(const std::shared_ptr<ssl_socket>& socket, const json& payload) {
    sendFramed(*socket, payload.dump());
}

void broadcast(const json& payload) {
    std::string data = payload.dump();
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto it = clients.begin(); it != clients.end();) {
        try {
            sendFramed(**it, data);
            ++it;
        } catch (const std::exception&) {
            it = clients.erase(it);
        }
    }
}

void removeClient(const std::shared_ptr<ssl_socket>& client) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
}

void handleRequest(const std::shared_ptr<ssl_socket>& socket, const json& request) {
    std::string command = request.value("command", "");

    if (command == "add") {
        std::string author = request.value("author", "");
        std::string text = request.value("text", "");

        if (author.empty() || text.empty()) {
            sendTo(socket, {{"type", "error"}, {"message", "author and text are required"}});
            return;
        }

        json newMessageJson;
        {
            // storage->insert() пишет в БД и возвращает уже готовое сообщение
            // с настоящими id/датой, назначенными базой (SERIAL, DEFAULT now()) —
            // store.addLoaded() здесь только кэширует его в памяти для быстрого
            // list/search, не выдумывая свои id.
            Message newMessage = storage->insert(author, text);

            std::lock_guard<std::mutex> lock(storeMutex);
            store.addLoaded(newMessage);
            newMessageJson = messageToJson(newMessage);
        }

        broadcast({{"type", "new_message"}, {"message", newMessageJson}});
    } else if (command == "list") {
        json list = json::array();
        {
            std::lock_guard<std::mutex> lock(storeMutex);
            for (const Message& message : store.all()) {
                list.push_back(messageToJson(message));
            }
        }
        sendTo(socket, {{"type", "messages"}, {"messages", list}});
    } else if (command == "search") {
        std::string query = request.value("query", "");

        json list = json::array();
        {
            std::lock_guard<std::mutex> lock(storeMutex);
            for (const Message& message : searchMessages(query, store.all())) {
                list.push_back(messageToJson(message));
            }
        }
        sendTo(socket, {{"type", "messages"}, {"messages", list}});
    } else {
        sendTo(socket, {{"type", "error"}, {"message", "unknown command: " + command}});
    }
}

void handleClient(std::shared_ptr<ssl_socket> socket) {
    try {
        socket->handshake(asio::ssl::stream_base::server);
    } catch (const std::exception& error) {
        std::cout << "TLS handshake failed: " << error.what() << '\n';
        return;
    }

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.push_back(socket);
    }

    std::cout << "Client connected. Total: " << clients.size() << '\n';

    try {
        while (true) {
            std::string raw = receiveFramed(*socket);

            json request;
            try {
                request = json::parse(raw);
            } catch (const json::parse_error&) {
                sendTo(socket, {{"type", "error"}, {"message", "invalid JSON"}});
                continue;
            }

            handleRequest(socket, request);
        }
    } catch (const std::exception& error) {
        std::cout << "Client disconnected: " << error.what() << '\n';
    }

    removeClient(socket);
}

int main() {
    try {
        std::ifstream configFile("db_config.json");
        if (!configFile.is_open()) {
            std::cerr << "db_config.json not found (see db_config.example.json)\n";
            return 1;
        }

        json config;
        configFile >> config;
        storage = std::make_unique<PostgresStorage>(config.at("connection_string").get<std::string>());

        for (Message& message : storage->loadAll()) {
            store.addLoaded(std::move(message));
        }

        asio::io_context io;
        asio::ssl::context tlsContext(asio::ssl::context::tls_server);

        tlsContext.set_options(
            asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3 |
            asio::ssl::context::no_tlsv1 |
            asio::ssl::context::no_tlsv1_1
        );
        tlsContext.use_certificate_chain_file("certs/server.crt");
        tlsContext.use_private_key_file("certs/server.key", asio::ssl::context::pem);

        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 5555));

        std::cout << "Server listening on port 5555 (TLS)\n";

        while (true) {
            auto socket = std::make_shared<ssl_socket>(io, tlsContext);
            acceptor.accept(socket->lowest_layer());

            std::thread(handleClient, socket).detach();
        }
    } catch (const std::exception& error) {
        std::cerr << "Server error: " << error.what() << '\n';
        return 1;
    }
}
