#include "chat_input_edit.h"

#include <QKeyEvent>

ChatInputEdit::ChatInputEdit(QWidget* parent) : QPlainTextEdit(parent) {
    setTabChangesFocus(true);
    // Ограничиваем высоту ~4 строками — дальше появляется собственный скролл
    // виджета, поле ввода не разрастается бесконечно и не сжимает остальной UI.
    setMaximumHeight(96);
}

void ChatInputEdit::keyPressEvent(QKeyEvent* event) {
    bool isEnter = (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter);
    if (isEnter && !(event->modifiers() & Qt::ShiftModifier)) {
        emit sendRequested();
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}
