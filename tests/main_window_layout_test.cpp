#include <QApplication>
#include <QWidget>

#include <iostream>

#include "main_window.h"
#include "network_client.h"

namespace {
bool require(bool condition, const char* message) {
    if (condition) {
        return true;
    }

    std::cerr << message << '\n';
    return false;
}
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    NetworkClient client;
    MainWindow window(&client, "layout_test");
    window.setAttribute(Qt::WA_DontShowOnScreen);
    window.resize(760, 680);
    window.show();
    app.processEvents();

    QWidget* header = window.findChild<QWidget*>("chatHeader");
    QWidget* search = window.findChild<QWidget*>("searchBar");
    QWidget* messages = window.findChild<QWidget*>("messageList");
    QWidget* composer = window.findChild<QWidget*>("messageComposer");

    if (!require(header != nullptr, "Chat header is missing") ||
        !require(search != nullptr, "Search bar is missing") ||
        !require(messages != nullptr, "Message feed is missing") ||
        !require(composer != nullptr, "Message composer is missing")) {
        return 1;
    }

    if (!require(header->geometry().bottom() < search->geometry().top(),
                 "Search must be below the header") ||
        !require(search->geometry().bottom() < messages->geometry().top(),
                 "Message feed must be below search") ||
        !require(messages->geometry().bottom() < composer->geometry().top(),
                 "Message composer must be below the feed")) {
        return 1;
    }

    return 0;
}
