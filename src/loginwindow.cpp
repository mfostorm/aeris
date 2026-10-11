#include "loginwindow.h"
#include "database.h"
#include "registerdialog.h"
#include "guestwindow.h"
#include "adminwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("ИС «Космос» — вход в систему");
    setFixedSize(460, 520);
    setStyleSheet("LoginWindow { background: #f3f5f9; }");

    auto *card = new QFrame;
    card->setObjectName("card");
    auto *v = new QVBoxLayout(card);
    v->setContentsMargins(32, 28, 32, 28);
    v->setSpacing(8);

    auto *title = new QLabel("Вход в систему");
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    auto *sub = new QLabel("Гостиница «Космос»");
    sub->setObjectName("subtitle");
    sub->setAlignment(Qt::AlignCenter);

    m_email = new QLineEdit;
    m_email->setPlaceholderText("user@example.com");
    m_password = new QLineEdit;
    m_password->setPlaceholderText("Пароль");
    m_password->setEchoMode(QLineEdit::Password);

    m_error = new QLabel;
    m_error->setStyleSheet("color: #dc2626;");
    m_error->setAlignment(Qt::AlignCenter);
    m_error->hide();

    auto *btnLogin = new QPushButton("Войти");
    auto *btnReg = new QPushButton("Зарегистрироваться");
    btnReg->setObjectName("link");
    auto *hint = new QLabel("Вход администратора: admin@kosmos.ru");
    hint->setObjectName("subtitle");
    hint->setAlignment(Qt::AlignCenter);

    v->addWidget(title);
    v->addWidget(sub);
    v->addSpacing(14);
    v->addWidget(new QLabel("Email"));
    v->addWidget(m_email);
    v->addWidget(new QLabel("Пароль"));
    v->addWidget(m_password);
    v->addWidget(m_error);
    v->addSpacing(6);
    v->addWidget(btnLogin);
    auto *h = new QHBoxLayout;
    h->addStretch();
    h->addWidget(new QLabel("Нет аккаунта?"));
    h->addWidget(btnReg);
    h->addStretch();
    v->addLayout(h);
    v->addWidget(hint);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(36, 36, 36, 36);
    outer->addWidget(card);

    connect(btnLogin, &QPushButton::clicked, this, [this] { tryLogin(); });
    connect(m_password, &QLineEdit::returnPressed, this, [this] { tryLogin(); });
    connect(btnReg, &QPushButton::clicked, this, [this] {
        RegisterDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted)
            fill(dlg.email(), "");
    });
}

void LoginWindow::fill(const QString &email, const QString &password)
{
    m_email->setText(email);
    m_password->setText(password);
}

bool LoginWindow::tryLogin(bool openWindow)
{
    // проверка заполненности полей
    if (m_email->text().trimmed().isEmpty() || m_password->text().isEmpty()) {
        m_error->setText("Введите email и пароль");
        m_error->show();
        return false;
    }
    User user;
    if (!Database::login(m_email->text(), m_password->text(), user)) {
        m_error->setText("Неверный email или пароль");
        m_error->show();
        return false;
    }
    m_error->hide();
    if (!openWindow)
        return true;

    // открытие окна в зависимости от роли пользователя
    QWidget *w = nullptr;
    if (user.role == "admin")
        w = new AdminWindow(user);
    else
        w = new GuestWindow(user);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    hide();
    connect(w, &QObject::destroyed, this, [this] { m_password->clear(); show(); });
    return true;
}
