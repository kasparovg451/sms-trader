#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

// Простой диалог входа: имя пользователя + пароль, кнопки "Войти" и
// "Регистрация". Капча и т.п. сознательно отложены — см. принцип
// "правильно, но не переуглубляясь" в ROADMAP.
class LoginDialog : public QDialog {
    Q_OBJECT

public:
    enum class Action { Login, Register };

    explicit LoginDialog(QWidget* parent = nullptr);

    // Чтобы при повторной попытке после ошибки не заставлять перепечатывать
    // имя — только пароль. Пароль намеренно не предзаполняется никогда.
    void setUsername(const QString& username);

    QString username() const;
    QString password() const;
    Action action() const { return action_; }

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    Action action_ = Action::Login;
};
