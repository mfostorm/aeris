#include "registerdialog.h"
#include "database.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>

RegisterDialog::RegisterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("ИС «Космос» — регистрация");
    setFixedSize(460, 600);

    auto *card = new QFrame;
    card->setObjectName("card");
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(32, 24, 32, 24);
    v->setSpacing(7);

    auto *title = new QLabel("Регистрация");
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);

    m_name = new QLineEdit;  m_name->setPlaceholderText("Иван Иванов");
    m_email = new QLineEdit; m_email->setPlaceholderText("user@example.com");
    m_pass = new QLineEdit;  m_pass->setEchoMode(QLineEdit::Password); m_pass->setPlaceholderText("не менее 6 символов");
    m_pass2 = new QLineEdit; m_pass2->setEchoMode(QLineEdit::Password);
    m_error = new QLabel;
    m_error->setStyleSheet("color: #dc2626;");
    m_error->setWordWrap(true);
    m_error->hide();

    auto *btn = new QPushButton("Создать аккаунт");
    auto *back = new QPushButton("Уже есть аккаунт? Войти");
    back->setObjectName("link");

    v->addWidget(title);
    v->addSpacing(8);
    v->addWidget(new QLabel("Ваше имя"));       v->addWidget(m_name);
    v->addWidget(new QLabel("Email"));          v->addWidget(m_email);
    v->addWidget(new QLabel("Пароль"));         v->addWidget(m_pass);
    v->addWidget(new QLabel("Повторите пароль")); v->addWidget(m_pass2);
    v->addWidget(m_error);
    v->addSpacing(6);
    v->addWidget(btn);
    v->addWidget(back, 0, Qt::AlignCenter);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(36, 30, 36, 30);
    outer->addWidget(card);

    connect(btn, &QPushButton::clicked, this, &RegisterDialog::onRegister);
    connect(back, &QPushButton::clicked, this, &QDialog::reject);
}

QString RegisterDialog::email() const { return m_email->text().trimmed(); }

void RegisterDialog::fill(const QString &name, const QString &email, const QString &pass)
{
    m_name->setText(name); m_email->setText(email);
    m_pass->setText(pass); m_pass2->setText(pass);
}

void RegisterDialog::onRegister()
{
    QString err;
    const QString email = m_email->text().trimmed();
    // проверка введённых данных
    if (m_name->text().trimmed().length() < 2)
        err = "Введите имя (не менее 2 символов)";
    else if (!email.contains('@') || !email.contains('.'))
        err = "Некорректный email";
    else if (m_pass->text().length() < 6)
        err = "Пароль должен содержать не менее 6 символов";
    else if (m_pass->text() != m_pass2->text())
        err = "Пароли не совпадают";
    else if (Database::emailExists(email))
        err = "Пользователь с таким email уже зарегистрирован";

    if (!err.isEmpty()) {
        m_error->setText(err);
        m_error->show();
        return;
    }
    if (Database::registerGuest(m_name->text(), email, m_pass->text())) {
        QMessageBox::information(this, "Регистрация", "Учётная запись создана. Выполните вход.");
        accept();
    }
}
