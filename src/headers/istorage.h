#pragma once

#include <string>
#include <vector>

#include "message.h"

// Абстракция хранилища — сервер работает с ней, не зная, JSON-файл под
// капотом или настоящая база данных. Позволяет подменить реализацию,
// не трогая код сервера.
class IStorage {
public:
    virtual ~IStorage() = default;

    virtual std::vector<Message> loadAll() = 0;

    // Хранилище само назначает id и время создания (для БД — через
    // SERIAL/DEFAULT now()) и возвращает уже полностью заполненное
    // сообщение — вызывающий код не должен сам выдумывать эти поля.
    virtual Message insert(const std::string& author, const std::string& text) = 0;
};
