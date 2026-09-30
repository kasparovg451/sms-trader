#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include <fstream>

#include "login_dialog.h"
#include "main_window.h"
#include "network_client.h"
#include "app_config.h"

namespace {
const char* kStyleSheet = R"(
    QMainWindow, QWidget#central { background: #101722; }
    QDialog { background: #101722; }
    QLabel { color: #edf4ff; }
    QWidget#chatHeader { background: #172231; border-bottom: 1px solid #27374d; }
    QLabel#headerTitle { color: #f4f8ff; font-size: 17px; font-weight: 700; }
    QLabel#headerSubtitle { color: #8aa0bd; font-size: 11px; }
    QLabel#accountLabel { color: #8fd1b0; font-size: 12px; font-weight: 600; }
    QWidget#searchBar { background: #141d2a; border-bottom: 1px solid #223147; }
    QWidget#messageComposer { background: #172231; border-top: 1px solid #27374d; }
    QListWidget { border: none; background: #101722; padding: 10px 16px; }
    QListWidget::item { border: none; }
    QFrame#ownBubble {
        background: #2b5f9b;
        border-radius: 12px;
    }
    QFrame#otherBubble {
        background: #202d40;
        border: 1px solid #2a3c56;
        border-radius: 12px;
    }
    QLabel#authorLabel { color: #8fc4ff; font-weight: bold; font-size: 12px; }
    QLabel#dateLabel { color: #9eb0c8; font-size: 10px; }
    QLabel#textLabel { color: #f0f5ff; font-size: 13px; }
    QLabel#emptyStateLabel { color: #9eb0c8; font-size: 13px; padding: 24px; }
    QLabel#dateSeparator {
        background: #233249;
        color: #b6c5dc;
        font-size: 11px;
        font-weight: bold;
        padding: 3px 12px;
        border-radius: 9px;
    }
    QLineEdit, QPlainTextEdit {
        border: 1px solid #31435e;
        border-radius: 10px;
        padding: 8px 12px;
        background: #1d2a3c;
        color: #eef4ff;
        font-size: 13px;
    }
    QLineEdit:focus, QPlainTextEdit:focus { border-color: #5798ff; }
    QLineEdit#searchInput { background: #1a2637; }
    QPushButton {
        background: #4f91f4;
        color: white;
        border-radius: 10px;
        padding: 8px 18px;
        border: none;
        font-size: 13px;
    }
    QPushButton:hover { background: #6aa4ff; }
    QPushButton:pressed { background: #377bd7; }
    QPushButton#searchButton { background: #263951; color: #c5d5ec; }
    QPushButton#searchButton:hover { background: #304a69; }
    QPushButton#showAllButton { background: transparent; color: #8fc4ff; }
    QPushButton#sendButton { min-width: 84px; font-weight: 700; }
    QStatusBar { background: #141d2a; color: #8fa4bf; }
)";
}

int main(int argc, char* argv[]) {
    std::ofstream crashLog("gui_crash.log", std::ios::app);

    try {
        QApplication app(argc, argv);
        // Родной стиль Windows (windowsvista/windows11) рисует виджеты через
        // системную тему и игнорирует большую часть QSS-стилей. Fusion — свой
        // движок рендеринга Qt, полностью уважающий стили.
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        // Стиль применяем на уровне приложения, не отдельного окна — по
        // не до конца понятной причине QSS на конкретном QMainWindow здесь
        // не применялся, хотя программно был установлен корректно
        // (проверено логированием). На уровне QApplication работает надёжно.
        app.setStyleSheet(kStyleSheet);

        const AppConfig config = loadAppConfig("app_config.json");
        NetworkClient client(QString::fromStdString(config.apiBaseUrl));

        // Цикл: при неверном пароле/занятом имени и т.п. снова показываем
        // диалог, а не падаем — пользователь мог просто опечататься.
        // lastUsername сохраняет введённое имя между попытками — не заставляем
        // перепечатывать его заново после одной лишь ошибки в пароле.
        QString lastUsername;
        while (true) {
            LoginDialog dialog;
            if (!lastUsername.isEmpty()) {
                dialog.setUsername(lastUsername);
            }
            if (dialog.exec() != QDialog::Accepted) {
                return 0;
            }

            QString username = dialog.username();
            QString password = dialog.password();
            lastUsername = username;
            if (username.isEmpty() || password.isEmpty()) {
                QMessageBox::warning(nullptr, "SMSTrader", "Имя пользователя и пароль обязательны");
                continue;
            }

            QString error;
            if (dialog.action() == LoginDialog::Action::Register) {
                if (!client.registerUser(username, password, error)) {
                    QMessageBox::warning(nullptr, "Ошибка регистрации", error);
                    continue;
                }
            }

            if (!client.login(username, password, error)) {
                QMessageBox::warning(nullptr, "Ошибка входа", error);
                continue;
            }

            client.connectRealtime();

            MainWindow window(&client, username);
            window.show();

            return app.exec();
        }
    } catch (const std::exception& error) {
        crashLog << "Uncaught std::exception: " << error.what() << std::endl;
        return 2;
    } catch (...) {
        crashLog << "Uncaught unknown exception" << std::endl;
        return 2;
    }
}
