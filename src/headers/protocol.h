#pragma once

#include <array>
#include <asio.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>

// Framing по длине: перед каждым сообщением идёт 4-байтовый заголовок с длиной
// в big-endian ("сетевой порядок байт"), затем сами байты сообщения.
// Бинарно-безопасно — в отличие от разделителя '\n', сообщение может содержать
// что угодно, включая переносы строк.

constexpr uint32_t maxMessageLength = 1'000'000; // защита от чужого мусора/DoS

inline std::array<unsigned char, 4> encodeLength(uint32_t length) {
    return {
        static_cast<unsigned char>((length >> 24) & 0xFF),
        static_cast<unsigned char>((length >> 16) & 0xFF),
        static_cast<unsigned char>((length >> 8) & 0xFF),
        static_cast<unsigned char>(length & 0xFF)
    };
}

inline uint32_t decodeLength(const std::array<unsigned char, 4>& bytes) {
    return (static_cast<uint32_t>(bytes[0]) << 24) |
           (static_cast<uint32_t>(bytes[1]) << 16) |
           (static_cast<uint32_t>(bytes[2]) << 8) |
           static_cast<uint32_t>(bytes[3]);
}

// Шаблон по типу потока — работает и с tcp::socket, и позже с asio::ssl::stream
// без единой правки, когда добавим TLS.
template <typename SyncWriteStream>
void sendFramed(SyncWriteStream& stream, const std::string& payload) {
    auto header = encodeLength(static_cast<uint32_t>(payload.size()));
    asio::write(stream, asio::buffer(header));
    asio::write(stream, asio::buffer(payload));
}

template <typename SyncReadStream>
std::string receiveFramed(SyncReadStream& stream) {
    std::array<unsigned char, 4> header{};
    asio::read(stream, asio::buffer(header));

    uint32_t length = decodeLength(header);
    if (length > maxMessageLength) {
        throw std::runtime_error("Message too large");
    }

    std::string payload(length, '\0');
    asio::read(stream, asio::buffer(payload));
    return payload;
}
