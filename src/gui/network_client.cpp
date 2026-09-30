#include "network_client.h"
#include "auth_response.h"

#include <QEventLoop>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslCertificate>
#include <QUrlQuery>
#include <nlohmann/json.hpp>

#include <utility>

using json = nlohmann::json;

namespace {

QVariantMap toVariantMap(const json& message) {
    QVariantMap map;
    map["id"] = message.value("id", 0);
    map["author"] = QString::fromStdString(message.value("author", ""));
    map["text"] = QString::fromStdString(message.value("text", ""));
    map["relativeDate"] = QString::fromStdString(message.value("relativeDate", ""));
    map["timestamp"] = static_cast<qlonglong>(message.value("timestamp", 0LL));
    return map;
}

}  // namespace

NetworkClient::NetworkClient(QString apiBaseUrl, QObject* parent)
    : QObject(parent), apiBaseUrl_(std::move(apiBaseUrl)) {
    connect(&socket_, &QWebSocket::connected, this, &NetworkClient::onSocketConnected);
    connect(&socket_, &QWebSocket::textMessageReceived, this, &NetworkClient::onSocketTextMessage);
    connect(&socket_, &QWebSocket::disconnected, this, &NetworkClient::onSocketDisconnected);
    connect(&socket_, &QWebSocket::errorOccurred, this, &NetworkClient::onSocketError);

    reconnectTimer_.setSingleShot(true);
    connect(&reconnectTimer_, &QTimer::timeout, this, &NetworkClient::connectRealtime);
}

QString NetworkClient::baseUrl() const {
    return apiBaseUrl_;
}

QSslConfiguration NetworkClient::sslConfigTrustingServerCert() const {
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    QFile certFile("certs/server.crt");
    if (certFile.open(QIODevice::ReadOnly)) {
        QSslCertificate certificate(&certFile, QSsl::Pem);
        // Наш сертификат самоподписанный — доверяем ему явно как корню,
        // а не отключаем проверку целиком (verify_none мы отвергли ещё
        // на Этапе 3, см. журнал решений — тот же принцип и здесь).
        config.setCaCertificates({certificate});
    }
    return config;
}

bool NetworkClient::login(const QString& username, const QString& password, QString& errorMessage) {
    QNetworkRequest request(QUrl(baseUrl() + "/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setSslConfiguration(sslConfigTrustingServerCert());

    json body{{"username", username.toStdString()}, {"password", password.toStdString()}};
    QByteArray payload = QByteArray::fromStdString(body.dump());

    QNetworkReply* reply = network_.post(request, payload);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseBody = reply->readAll();
    const std::string transportError = reply->error() == QNetworkReply::NoError
        ? std::string{}
        : reply->errorString().toStdString();
    reply->deleteLater();

    const AuthResponse response = parseLoginResponse(statusCode, responseBody.toStdString(), transportError);
    if (!response.success) {
        errorMessage = QString::fromStdString(response.error);
        return false;
    }

    jwtToken_ = QString::fromStdString(response.token);
    errorMessage.clear();
    return true;
}

bool NetworkClient::registerUser(const QString& username, const QString& password, QString& errorMessage) {
    QNetworkRequest request(QUrl(baseUrl() + "/register"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setSslConfiguration(sslConfigTrustingServerCert());

    json body{{"username", username.toStdString()}, {"password", password.toStdString()}};
    QByteArray payload = QByteArray::fromStdString(body.dump());

    QNetworkReply* reply = network_.post(request, payload);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseBody = reply->readAll();
    const std::string transportError = reply->error() == QNetworkReply::NoError
        ? std::string{}
        : reply->errorString().toStdString();
    reply->deleteLater();

    const AuthResponse response = parseRegistrationResponse(
        statusCode,
        responseBody.toStdString(),
        transportError
    );
    if (!response.success) {
        errorMessage = QString::fromStdString(response.error);
        return false;
    }

    errorMessage.clear();
    return true;
}

void NetworkClient::connectRealtime() {
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        // Уже подключены или подключаемся — повторный open() тут не нужен
        // (может случиться, если reconnectTimer_ выстрелит одновременно
        // с ручным вызовом извне).
        return;
    }
    socket_.setSslConfiguration(sslConfigTrustingServerCert());
    // Токен — заголовком при рукопожатии (сервер проверяет в onaccept), а не
    // в ?token= — так он не попадёт в логи прокси. QWebSocket::open(QUrl)
    // заголовки ставить не умеет, нужна перегрузка с QNetworkRequest.
    QUrl realtimeUrl(apiBaseUrl_);
    realtimeUrl.setScheme(realtimeUrl.scheme() == "https" ? "wss" : "ws");
    realtimeUrl.setPath("/ws");
    realtimeUrl.setQuery(QString());
    QNetworkRequest request(realtimeUrl);
    request.setRawHeader("Authorization", "Bearer " + jwtToken_.toUtf8());
    socket_.open(request);
}

void NetworkClient::onSocketConnected() {
    // Список не подтягиваем здесь: пока нас не было, могли прилететь
    // сообщения (WS не досылает их задним числом), но актуальный список
    // нужен по-разному в зависимости от того, смотрит ли пользователь
    // сейчас на поиск или на общую ленту — это знает MainWindow, не мы.
    emit connectedToServer();
}

void NetworkClient::onSocketTextMessage(const QString& message) {
    json response;
    try {
        response = json::parse(message.toStdString());
    } catch (const json::parse_error&) {
        emit errorReceived("Malformed realtime message from server");
        return;
    }
    // Сервер сейчас шлёт в /ws только один тип полезной нагрузки — новое
    // сообщение (плоский объект, без обёртки {"type": ...}, см. src/http/main.cpp).
    emit messageAdded(toVariantMap(response));
}

void NetworkClient::onSocketDisconnected() {
    emit disconnectedFromServer();
    // Простая фиксированная задержка, без экспоненциального backoff — сервер
    // локальный, для принципа "не переуглубляясь" этого достаточно; если станет
    // мешать при реальном простое сервера, вернёмся и сделаем backoff.
    reconnectTimer_.start(3000);
}

void NetworkClient::onSocketError() {
    emit errorReceived(socket_.errorString());
}

void NetworkClient::sendAdd(const QString& text) {
    QNetworkRequest request = authorizedRequest(QUrl(baseUrl() + "/messages"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    json body{{"text", text.toStdString()}};
    QNetworkReply* reply = network_.post(request, QByteArray::fromStdString(body.dump()));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode != 201) {
            json response;
            try {
                response = json::parse(reply->readAll().toStdString());
                emit errorReceived(QString::fromStdString(response.value("error", "не удалось отправить сообщение")));
            } catch (const json::parse_error&) {
                emit errorReceived("не удалось отправить сообщение");
            }
        }
        // При успехе (201) саму отправку в ленту не показываем: сервер
        // разошлёт это же сообщение всем подписчикам /ws, включая нас самих —
        // появится в ленте оттуда.
        emit messageSendFinished();
        reply->deleteLater();
    });
}

void NetworkClient::handleMessageListReply(QNetworkReply* reply) {
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    json response;
    try {
        response = json::parse(reply->readAll().toStdString());
    } catch (const json::parse_error&) {
        emit errorReceived("Malformed response from server");
        reply->deleteLater();
        return;
    }
    reply->deleteLater();

    // При ошибке (401 — токен протух, 5xx) сервер отдаёт объект {"error": ...},
    // не массив — итерироваться по нему как по списку сообщений нельзя.
    if (statusCode != 200 || !response.is_array()) {
        std::string error = response.is_object() ? response.value("error", "server error") : "server error";
        emit errorReceived(QString::fromStdString(error));
        return;
    }

    QVariantList messages;
    for (const json& message : response) {
        messages << toVariantMap(message);
    }
    emit listReceived(messages);
}

QNetworkRequest NetworkClient::authorizedRequest(const QUrl& url) const {
    QNetworkRequest request(url);
    request.setSslConfiguration(sslConfigTrustingServerCert());
    request.setRawHeader("Authorization", "Bearer " + jwtToken_.toUtf8());
    return request;
}

void NetworkClient::sendList() {
    QNetworkReply* reply = network_.get(authorizedRequest(QUrl(baseUrl() + "/messages")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleMessageListReply(reply); });
}

void NetworkClient::sendSearch(const QString& query) {
    QUrl url(baseUrl() + "/messages/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    url.setQuery(urlQuery);

    QNetworkReply* reply = network_.get(authorizedRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleMessageListReply(reply); });
}
