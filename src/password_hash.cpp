#include "headers/password_hash.h"

#include <argon2.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <vector>

namespace {
// Параметры Argon2id: время, память, параллелизм — компромисс между
// стойкостью и скоростью входа. 64 МБ памяти — достаточно, чтобы GPU-перебор
// был дорогим, но не заставлять пользователя ждать секундами при входе.
constexpr uint32_t kTimeCost = 3;
constexpr uint32_t kMemoryCostKiB = 1 << 16; // 64 МБ
constexpr uint32_t kParallelism = 1;
constexpr size_t kSaltLength = 16;
constexpr size_t kHashLength = 32;
} // namespace

std::string hashPassword(const std::string& password) {
    std::vector<unsigned char> salt(kSaltLength);
    // RAND_bytes (OpenSSL), не std::random_device — уже используем OpenSSL
    // в проекте, и RAND_bytes даёт явную гарантию криптографической стойкости
    // на всех платформах (у std::random_device такой гарантии формально нет).
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }

    size_t encodedLength = argon2_encodedlen(
        kTimeCost, kMemoryCostKiB, kParallelism, kSaltLength, kHashLength, Argon2_id
    );
    std::vector<char> encoded(encodedLength);

    int result = argon2id_hash_encoded(
        kTimeCost, kMemoryCostKiB, kParallelism,
        password.data(), password.size(),
        salt.data(), salt.size(),
        kHashLength, encoded.data(), encoded.size()
    );

    if (result != ARGON2_OK) {
        throw std::runtime_error(std::string("Argon2 hashing failed: ") + argon2_error_message(result));
    }

    return std::string(encoded.data());
}

bool verifyPassword(const std::string& password, const std::string& encodedHash) {
    int result = argon2id_verify(encodedHash.c_str(), password.data(), password.size());
    return result == ARGON2_OK;
}
