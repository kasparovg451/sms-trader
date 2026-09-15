#pragma once

#include <QDate>
#include <QMainWindow>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QLineEdit;
class QListWidget;
class QPushButton;
class NetworkClient;
class ChatInputEdit;

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
    void addDateSeparatorIfNeeded(qint64 timestamp);

    NetworkClient* client_;
    QString myAuthor_;
    bool searchMode_ = false;
    // true, пока в ленте лежит только "Сообщений пока нет..." — следующее
    // добавленное сообщение (свежее или из списка) должно сначала её убрать.
    bool showingEmptyState_ = false;
    // UTC-дата последнего добавленного сообщения — когда день меняется,
    // перед следующим сообщением вставляется разделитель ("Сегодня" и т.п.).
    bool hasLastMessageDate_ = false;
    QDate lastMessageDate_;
    // Без этого onMessageSendFinished мог бы разблокировать sendButton_ поверх
    // состояния "нет связи", если ответ на отправку придёт уже после обрыва WS.
    bool connected_ = true;
    QListWidget* messageList_;
    ChatInputEdit* messageEdit_;
    QPushButton* sendButton_;
    QLineEdit* searchEdit_;
    QPushButton* searchButton_;
    QPushButton* showAllButton_;
};
