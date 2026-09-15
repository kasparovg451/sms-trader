#include "login_dialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("SMSTrader — вход");
    setModal(true);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Имя пользователя:", this));
    usernameEdit_ = new QLineEdit(this);
    layout->addWidget(usernameEdit_);

    layout->addWidget(new QLabel("Пароль (минимум 8 символов):", this));
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit_);

    auto* buttons = new QDialogButtonBox(this);
    QPushButton* loginButton = buttons->addButton("Войти", QDialogButtonBox::AcceptRole);
    QPushButton* registerButton = buttons->addButton("Регистрация", QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Enter в поле пароля — как нажатие "Войти", самый частый путь.
    connect(passwordEdit_, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

QString LoginDialog::username() const {
    return usernameEdit_->text().trimmed();
}

QString LoginDialog::password() const {
    return passwordEdit_->text();
}

void LoginDialog::onLoginClicked() {
    action_ = Action::Login;
    accept();
}

void LoginDialog::onRegisterClicked() {
    action_ = Action::Register;
    accept();
}
