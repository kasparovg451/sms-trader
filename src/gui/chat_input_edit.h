#pragma once

#include <QPlainTextEdit>

// QPlainTextEdit с чат-семантикой Enter: Enter отправляет (сигнал
// sendRequested), Shift+Enter вставляет перенос строки. Обычному QLineEdit
// многострочность вообще недоступна, а голому QPlainTextEdit, наоборот,
// Enter всегда просто переносит строку — для чата нужно наоборот.
class ChatInputEdit : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit ChatInputEdit(QWidget* parent = nullptr);

signals:
    void sendRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
};
