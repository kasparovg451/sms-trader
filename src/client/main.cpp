#include <asio.hpp>
#include <asio/ssl.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#include "headers/protocol.h"

using asio::ip::tcp;
using ssl_socket = asio::ssl::stream<tcp::socket>;
using json = nlohmann::json;

void printOneMessage(const json& message) {
    std::cout << message.value("id", 0) << " || " << message.value("author", "")
              << ": " << message.value("text", "") << " ("
              << message.value("relativeDate", "") << ")\n";
}

void receiveLoop(std::shared_ptr<ssl_socket> socket) {
    try {
        while (true) {
            std::string raw = receiveFramed(*socket);

            json response;
            try {
                response = json::parse(raw);
            } catch (const json::parse_error&) {
                std::cout << "\rGot malformed response from server\n> " << std::flush;
                continue;
            }

            std::string type = response.value("type", "");

            std::cout << "\r";
            if (type == "new_message") {
                printOneMessage(response["message"]);
            } else if (type == "messages") {
                for (const json& message : response["messages"]) {
                    printOneMessage(message);
                }
            } else if (type == "error") {
                std::cout << "Error: " << response.value("message", "") << '\n';
            } else {
                std::cout << "Unknown response type\n";
            }
            std::cout << "> " << std::flush;
        }
    } catch (const std::exception&) {
        std::cout << "\nDisconnected from server\n";
        std::exit(0);
    }
}

int main() {
    try {
        asio::io_context io;
        asio::ssl::context tlsContext(asio::ssl::context::tls_client);

        tlsContext.set_options(
            asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3 |
            asio::ssl::context::no_tlsv1 |
            asio::ssl::context::no_tlsv1_1
        );
        tlsContext.load_verify_file("certs/server.crt");

        auto socket = std::make_shared<ssl_socket>(io, tlsContext);
        socket->set_verify_mode(asio::ssl::verify_peer);
        socket->set_verify_callback(asio::ssl::host_name_verification("localhost"));

        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", "5555");
        asio::connect(socket->lowest_layer(), endpoints);
        socket->handshake(asio::ssl::stream_base::client);

        std::cout << "Connected to server (TLS). Commands: add, list, search, help, exit\n";

        std::thread(receiveLoop, socket).detach();

        std::string command;
        while (true) {
            std::cout << "\n> ";
            std::getline(std::cin, command);

            if (command == "exit") {
                break;
            } else if (command == "add") {
                std::string author;
                std::string text;

                std::cout << "Your name: ";
                std::getline(std::cin, author);

                std::cout << "Your message: ";
                std::getline(std::cin, text);

                sendFramed(*socket, json{{"command", "add"}, {"author", author}, {"text", text}}.dump());
            } else if (command == "list") {
                sendFramed(*socket, json{{"command", "list"}}.dump());
            } else if (command == "search") {
                std::string query;
                std::cout << "Search query: ";
                std::getline(std::cin, query);

                sendFramed(*socket, json{{"command", "search"}, {"query", query}}.dump());
            } else if (command == "help") {
                std::cout << "Commands: add, list, search, exit\n";
            } else {
                std::cout << "Unknown command: " << command << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Client error: " << error.what() << '\n';
        return 1;
    }
}
