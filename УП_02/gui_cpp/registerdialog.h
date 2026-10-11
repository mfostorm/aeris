// Окно регистрации нового гостя
#pragma once
#include <QDialog>
class QLineEdit;
class QLabel;

class RegisterDialog : public QDialog
{
public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    QString email() const;
    void fill(const QString &name, const QString &email, const QString &pass);

private:
    void onRegister();
    QLineEdit *m_name, *m_email, *m_pass, *m_pass2;
    QLabel *m_error;
};
