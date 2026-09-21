#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QSslConfiguration>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QWebSocket>

// Говорит с smstrader_http: REST (QNetworkAccessManager) для запрос/ответ-операций
// (регистрация, вход, список, поиск, отправка) и WebSocket (QWebSocket) для
// живой рассылки новых сообщений. Раньше GUI ходил напрямую в smstrader_server
// по своему TLS+бинарному протоколу — теперь это единственная точка входа,
// та же, что используют curl/браузер/что угодно ещё (см. ROADMAP, Этап 4/6).
class NetworkClient : public QObject {
    Q_OBJECT

public:
    explicit NetworkClient(QObject* parent = nullptr);

    // Обе синхронные (крутят локальный QEventLoop) — вызываются один раз при
    // старте приложения, до показа главного окна, так что блокировка на время
    // одного запроса не страшна и сильно упрощает main.cpp.
    bool login(const QString& username, const QString& password, QString& errorMessage);
    bool registerUser(const QString& username, const QString& password, QString& errorMessage);

    // Открывает wss://.../ws — вызывать после успешного login().
    void connectRealtime();

    void sendAdd(const QString& text);
    void sendList();
    void sendSearch(const QString& query);

signals:
    void messageAdded(QVariantMap message);
    void listReceived(QVariantList messages);
    void errorReceived(QString message);
    // WS-соединение упало — временно, не значит "выходи из приложения".
    void disconnectedFromServer();
    // Реально (пере)подключились и можем снова слать/принимать.
    void connectedToServer();
    // POST /messages завершился (успехом или ошибкой) — сигнал для UI,
    // чтобы разблокировать кнопку "Отправить" и не дать наплодить дублей
    // повторными кликами, пока первый запрос ещё летит.
    void messageSendFinished();

private slots:
    void onSocketConnected();
    void onSocketTextMessage(const QString& message);
    void onSocketDisconnected();
    void onSocketError();

private:
    QSslConfiguration sslConfigTrustingServerCert() const;
    QString baseUrl() const;
    // Запрос с Bearer-токеном и доверием серверному сертификату — для всего,
    // что после логина (сервер требует токен и на чтение, не только на запись).
    QNetworkRequest authorizedRequest(const QUrl& url) const;
    // Общий разбор ответа со списком сообщений (GET /messages и поиск).
    void handleMessageListReply(QNetworkReply* reply);

    QNetworkAccessManager network_;
    QWebSocket socket_;
    QString jwtToken_;
    // WS у Qt сам не переподключается — без этого таймера обрыв связи
    // (сон ноутбука, перезапуск сервера) означал бы "открой GUI заново".
    QTimer reconnectTimer_;
};
