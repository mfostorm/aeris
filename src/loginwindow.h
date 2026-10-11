// Окно авторизации пользователя
#pragma once
#include <QWidget>
class QLineEdit;
class QLabel;

class LoginWindow : public QWidget
{
public:
    explicit LoginWindow(QWidget *parent = nullptr);
    void fill(const QString &email, const QString &password); // для демонстрации
    bool tryLogin(bool openWindow = true);                     // проверка логина и пароля

private:
    QLineEdit *m_email;
    QLineEdit *m_password;
    QLabel *m_error;
};
