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

private:
    QWidget* buildMessageBubble(const QVariantMap& message);
    void appendMessage(const QVariantMap& message, bool forceScroll);

    NetworkClient* client_;
    QString myAuthor_;
    bool searchMode_ = false;
    QListWidget* messageList_;
    QLineEdit* messageEdit_;
    QPushButton* sendButton_;
    QLineEdit* searchEdit_;
    QPushButton* searchButton_;
    QPushButton* showAllButton_;
};
