#include "main_window.h"

#include <QApplication>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QScrollBar>
#include <QStatusBar>
#include <QTimeZone>
#include <QVBoxLayout>
#include <QWidget>
#include <chrono>

#include "chat_input_edit.h"
#include "headers/datetime.h"
#include "network_client.h"

MainWindow::MainWindow(NetworkClient* client, const QString& myAuthor, QWidget* parent)
    : QMainWindow(parent), client_(client), myAuthor_(myAuthor) {
    setWindowTitle("SMSTrader — " + myAuthor);
    resize(760, 720);
    setMinimumSize(620, 560);

    auto* central = new QWidget(this);
    central->setObjectName("central");
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QWidget(central);
    header->setObjectName("chatHeader");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 12, 20, 12);
    headerLayout->setSpacing(12);

    auto* titleColumn = new QVBoxLayout;
    titleColumn->setContentsMargins(0, 0, 0, 0);
    titleColumn->setSpacing(1);
    auto* title = new QLabel("smstrader", header);
    title->setObjectName("headerTitle");
    auto* subtitle = new QLabel("Global conversation", header);
    subtitle->setObjectName("headerSubtitle");
    titleColumn->addWidget(title);
    titleColumn->addWidget(subtitle);
    headerLayout->addLayout(titleColumn);
    headerLayout->addStretch();

    auto* account = new QLabel("●  " + myAuthor, header);
    account->setObjectName("accountLabel");
    headerLayout->addWidget(account);
    layout->addWidget(header);

    auto* searchRow = new QWidget(central);
    searchRow->setObjectName("searchBar");
    auto* searchLayout = new QHBoxLayout(searchRow);
    searchLayout->setContentsMargins(16, 9, 16, 9);
    searchLayout->setSpacing(8);
    searchEdit_ = new QLineEdit(searchRow);
    searchEdit_->setObjectName("searchInput");
    searchEdit_->setPlaceholderText("Search messages");
    searchButton_ = new QPushButton("Search", searchRow);
    searchButton_->setObjectName("searchButton");
    showAllButton_ = new QPushButton("Show all", searchRow);
    showAllButton_->setObjectName("showAllButton");
    showAllButton_->setVisible(false);
    searchLayout->addWidget(searchEdit_, 1);
    searchLayout->addWidget(searchButton_);
    searchLayout->addWidget(showAllButton_);
    layout->addWidget(searchRow);

    messageList_ = new QListWidget(central);
    messageList_->setObjectName("messageList");
    messageList_->setSelectionMode(QAbstractItemView::NoSelection);
    messageList_->setFocusPolicy(Qt::NoFocus);
    messageList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    layout->addWidget(messageList_, 1);

    auto* sendRow = new QWidget(central);
    sendRow->setObjectName("messageComposer");
    auto* sendLayout = new QHBoxLayout(sendRow);
    sendLayout->setContentsMargins(16, 11, 16, 14);
    sendLayout->setSpacing(8);
    messageEdit_ = new ChatInputEdit(sendRow);
    messageEdit_->setObjectName("messageInput");
    messageEdit_->setPlaceholderText("Write a message  •  Enter to send, Shift + Enter for a new line");
    sendButton_ = new QPushButton("Send", sendRow);
    sendButton_->setObjectName("sendButton");
    sendLayout->addWidget(messageEdit_, 1);
    sendLayout->addWidget(sendButton_);
    layout->addWidget(sendRow);

    setCentralWidget(central);

    connect(sendButton_, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(searchButton_, &QPushButton::clicked, this, &MainWindow::onSearchClicked);
    connect(showAllButton_, &QPushButton::clicked, this, &MainWindow::onShowAllClicked);
    connect(messageEdit_, &ChatInputEdit::sendRequested, this, &MainWindow::onSendClicked);
    connect(searchEdit_, &QLineEdit::returnPressed, this, &MainWindow::onSearchClicked);

    connect(client_, &NetworkClient::messageAdded, this, &MainWindow::onMessageAdded, Qt::QueuedConnection);
    connect(client_, &NetworkClient::listReceived, this, &MainWindow::onListReceived, Qt::QueuedConnection);
    connect(client_, &NetworkClient::errorReceived, this, &MainWindow::onErrorReceived, Qt::QueuedConnection);
    connect(client_, &NetworkClient::disconnectedFromServer, this, &MainWindow::onDisconnected, Qt::QueuedConnection);
    connect(client_, &NetworkClient::connectedToServer, this, &MainWindow::onReconnected, Qt::QueuedConnection);
    connect(client_, &NetworkClient::messageSendFinished, this, &MainWindow::onMessageSendFinished, Qt::QueuedConnection);

    messageEdit_->setFocus();
    statusBar()->showMessage("Загрузка сообщений...");
    client_->sendList();
}

QWidget* MainWindow::buildMessageBubble(const QVariantMap& message) {
    QString author = message.value("author").toString();
    QString text = message.value("text").toString();
    QString relativeDate = message.value("relativeDate").toString();
    bool isOwn = (author == myAuthor_);

    auto* bubble = new QFrame;
    bubble->setObjectName(isOwn ? "ownBubble" : "otherBubble");
    bubble->setMaximumWidth(320);

    auto* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(10, 6, 10, 6);
    bubbleLayout->setSpacing(2);

    if (!isOwn) {
        auto* authorLabel = new QLabel(author.toHtmlEscaped(), bubble);
        authorLabel->setObjectName("authorLabel");
        bubbleLayout->addWidget(authorLabel);
    }

    auto* textLabel = new QLabel(text.toHtmlEscaped(), bubble);
    textLabel->setObjectName("textLabel");
    textLabel->setWordWrap(true);
    // Позволяет выделять и копировать текст сообщения (правым кликом или
    // Ctrl+C) — QLabel с этим флагом сам даёт стандартное контекстное меню.
    textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubbleLayout->addWidget(textLabel);

    auto* dateLabel = new QLabel(relativeDate.toHtmlEscaped(), bubble);
    dateLabel->setObjectName("dateLabel");
    dateLabel->setAlignment(Qt::AlignRight);
    bubbleLayout->addWidget(dateLabel);

    auto* row = new QWidget;
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    if (isOwn) {
        rowLayout->addStretch();
        rowLayout->addWidget(bubble);
    } else {
        rowLayout->addWidget(bubble);
        rowLayout->addStretch();
    }

    return row;
}

void MainWindow::addDateSeparatorIfNeeded(qint64 timestamp) {
    QDate messageDate = QDateTime::fromSecsSinceEpoch(timestamp, QTimeZone::UTC).date();
    if (hasLastMessageDate_ && messageDate == lastMessageDate_) {
        return;
    }
    hasLastMessageDate_ = true;
    lastMessageDate_ = messageDate;

    std::chrono::year_month_day ymd{
        std::chrono::year{messageDate.year()},
        std::chrono::month{static_cast<unsigned>(messageDate.month())},
        std::chrono::day{static_cast<unsigned>(messageDate.day())}
    };
    std::string label = formatDaySeparator(std::chrono::sys_seconds{std::chrono::sys_days{ymd}});

    auto* pill = new QLabel(QString::fromStdString(label));
    pill->setObjectName("dateSeparator");
    pill->setAlignment(Qt::AlignCenter);

    auto* row = new QWidget;
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 8, 0, 8);
    rowLayout->addStretch();
    rowLayout->addWidget(pill);
    rowLayout->addStretch();

    auto* item = new QListWidgetItem();
    item->setFlags(Qt::NoItemFlags);
    messageList_->addItem(item);
    messageList_->setItemWidget(item, row);
    item->setSizeHint(row->sizeHint());
}

void MainWindow::showEmptyState() {
    messageList_->clear();
    showingEmptyState_ = true;
    hasLastMessageDate_ = false;

    auto* label = new QLabel("Сообщений пока нет — напишите первое!");
    label->setObjectName("emptyStateLabel");
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);

    auto* item = new QListWidgetItem();
    item->setFlags(Qt::NoItemFlags);
    messageList_->addItem(item);
    messageList_->setItemWidget(item, label);
    item->setSizeHint(label->sizeHint());
}

void MainWindow::appendMessage(const QVariantMap& message, bool forceScroll) {
    if (showingEmptyState_) {
        // Заглушка "сообщений пока нет" должна исчезнуть, как только
        // появляется хоть одно настоящее сообщение — иначе останется висеть
        // сверху ленты вперемешку с реальными сообщениями.
        messageList_->clear();
        showingEmptyState_ = false;
    }

    QScrollBar* scrollBar = messageList_->verticalScrollBar();
    bool wasAtBottom = forceScroll || scrollBar->value() >= scrollBar->maximum() - 4;

    if (!searchMode_) {
        // Результаты поиска отсортированы по релевантности, не по дате —
        // разделители дней там были бы бессмысленны (даты прыгали бы туда-сюда).
        addDateSeparatorIfNeeded(message.value("timestamp").toLongLong());
    }

    QWidget* row = buildMessageBubble(message);

    auto* item = new QListWidgetItem();
    messageList_->addItem(item);
    messageList_->setItemWidget(item, row);
    item->setSizeHint(row->sizeHint());

    if (wasAtBottom) {
        messageList_->scrollToBottom();
    }
}

void MainWindow::onSendClicked() {
    QString text = messageEdit_->toPlainText().trimmed();
    if (text.isEmpty()) {
        return;
    }

    // Блокируем кнопку до ответа сервера — иначе двойной клик/двойной Enter
    // до прихода первого ответа отправит одно и то же сообщение дважды.
    sendButton_->setEnabled(false);
    client_->sendAdd(text);
    messageEdit_->clear();
}

void MainWindow::onSearchClicked() {
    QString query = searchEdit_->text().trimmed();
    if (query.isEmpty()) {
        return;
    }

    searchMode_ = true;
    showAllButton_->setVisible(true);
    client_->sendSearch(query);
}

void MainWindow::onShowAllClicked() {
    searchMode_ = false;
    showAllButton_->setVisible(false);
    searchEdit_->clear();
    client_->sendList();
}

void MainWindow::onMessageAdded(QVariantMap message) {
    if (searchMode_) {
        // Не мешаем результаты поиска живой рассылкой — новое сообщение
        // всё равно появится, как только вернёмся к полной ленте.
        return;
    }
    appendMessage(message, false);

    QString author = message.value("author").toString();
    if (author != myAuthor_ && !isActiveWindow()) {
        // Окно свёрнуто/не в фокусе — мигаем значком в панели задач, как
        // системное уведомление. Полноценный toast — за рамки "не переуглубляясь".
        QApplication::alert(this);
    }
}

void MainWindow::onListReceived(QVariantList messages) {
    statusBar()->clearMessage();

    if (messages.isEmpty()) {
        showEmptyState();
        return;
    }

    messageList_->clear();
    showingEmptyState_ = false;
    // Полный ребилд ленты — разделители дат надо посчитать заново, а не
    // опираться на то, что было в ленте до этого (могли быть результаты
    // поиска, где даты вообще не проставлялись).
    hasLastMessageDate_ = false;
    for (const QVariant& message : messages) {
        appendMessage(message.toMap(), true);
    }
}

void MainWindow::onErrorReceived(QString message) {
    statusBar()->showMessage("Ошибка: " + message, 5000);
}

void MainWindow::onMessageSendFinished() {
    if (connected_) {
        sendButton_->setEnabled(true);
    }
}

void MainWindow::onDisconnected() {
    // Не навсегда: NetworkClient сам пробует переподключиться раз в 3 секунды
    // (см. network_client.cpp) — просто не даём слать сообщения, пока связи нет.
    connected_ = false;
    statusBar()->showMessage("Соединение потеряно, переподключаемся...");
    sendButton_->setEnabled(false);
    searchButton_->setEnabled(false);
}

void MainWindow::onReconnected() {
    connected_ = true;
    statusBar()->clearMessage();
    sendButton_->setEnabled(true);
    searchButton_->setEnabled(true);

    // Пока связи не было, могли прилететь сообщения — WS их не досылает
    // задним числом. Подтягиваем актуальный список, но не поверх поиска:
    // это молча подменило бы результаты поиска общей лентой.
    if (!searchMode_) {
        client_->sendList();
    }
}
