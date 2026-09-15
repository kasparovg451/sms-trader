#pragma once

#include <QMainWindow>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QLineEdit;
class QListWidget;
class QPushButton;
class NetworkClient;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(NetworkClient* client, const QString& myAuthor, QWidget* parent = nullptr);

private slots:
    void onSendClicked();
    void onSearchClicked();
    void onShowAllClicked();
    void onMessageAdded(QVariantMap message);
    void onListReceived(QVariantList messages);
    void onErrorReceived(QString message);
    void onDisconnected();
    void onReconnected();
    void onMessageSendFinished();

private:
    QWidget* buildMessageBubble(const QVariantMap& message);
    void appendMessage(const QVariantMap& message, bool forceScroll);
    void showEmptyState();

    NetworkClient* client_;
    QString myAuthor_;
    bool searchMode_ = false;
    // true, пока в ленте лежит только "Сообщений пока нет..." — следующее
    // добавленное сообщение (свежее или из списка) должно сначала её убрать.
    bool showingEmptyState_ = false;
    // Без этого onMessageSendFinished мог бы разблокировать sendButton_ поверх
    // состояния "нет связи", если ответ на отправку придёт уже после обрыва WS.
    bool connected_ = true;
    QListWidget* messageList_;
    QLineEdit* messageEdit_;
    QPushButton* sendButton_;
    QLineEdit* searchEdit_;
    QPushButton* searchButton_;
    QPushButton* showAllButton_;
};
