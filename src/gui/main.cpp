#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include <fstream>

#include "login_dialog.h"
#include "main_window.h"
#include "network_client.h"

namespace {
const char* kStyleSheet = R"(
    QMainWindow, QWidget#central { background: #eef1f5; }
    QDialog { background: #eef1f5; }
    QLabel { color: #1a1a1a; }
    QListWidget { border: none; background: #eef1f5; }
    QListWidget::item { border: none; }
    QFrame#ownBubble {
        background: #dcf8c6;
        border-radius: 12px;
    }
    QFrame#otherBubble {
        background: #ffffff;
        border: 1px solid #e2e5ea;
        border-radius: 12px;
    }
    QLabel#authorLabel { color: #2f6fed; font-weight: bold; font-size: 12px; }
    QLabel#dateLabel { color: #9aa0a6; font-size: 10px; }
    QLabel#textLabel { color: #1a1a1a; font-size: 13px; }
    QLabel#emptyStateLabel { color: #9aa0a6; font-size: 13px; padding: 24px; }
    QLineEdit {
        border: 1px solid #ccd2da;
        border-radius: 10px;
        padding: 8px 12px;
        background: white;
        color: #1a1a1a;
        font-size: 13px;
    }
    QPushButton {
        background: #2f6fed;
        color: white;
        border-radius: 10px;
        padding: 8px 18px;
        border: none;
        font-size: 13px;
    }
    QPushButton:hover { background: #255ecb; }
    QPushButton:pressed { background: #1c4aa3; }
    QStatusBar { color: #c0392b; }
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

        NetworkClient client;

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
