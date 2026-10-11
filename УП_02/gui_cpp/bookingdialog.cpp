#include "bookingdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QRegularExpression>

// стоимость дополнительных услуг, руб.
static const int BREAKFAST_PER_NIGHT = 600;
static const int TRANSFER = 1500;
static const int SPA = 2500;

BookingDialog::BookingDialog(const User &user, int roomId, const QDate &from, const QDate &to, QWidget *parent)
    : QDialog(parent), m_user(user), m_roomId(roomId)
{
    setWindowTitle("ИС «Космос» — оформление бронирования");
    setFixedSize(500, 600);

    QSqlQuery q;
    q.prepare("SELECT number, category, price FROM rooms WHERE id = ?");
    q.addBindValue(roomId);
    q.exec();
    QString roomText;
    if (q.next()) {
        roomText = QString("Номер %1 — %2").arg(q.value(0).toString(), q.value(1).toString());
        m_price = q.value(2).toInt();
    }

    auto *v = new QVBoxLayout(this);
    v->setContentsMargins(20, 16, 20, 16);
    auto *title = new QLabel("Оформление бронирования");
    title->setObjectName("title");
    auto *room = new QLabel(roomText + QString("\nЦена: %1 руб. за ночь").arg(m_price));
    room->setObjectName("subtitle");

    auto *g1 = new QGroupBox("Данные проживания");
    auto *f = new QFormLayout(g1);
    m_from = new QDateEdit(from);
    m_to = new QDateEdit(to);
    for (QDateEdit *e : {m_from, m_to}) { e->setCalendarPopup(true); e->setDisplayFormat("dd.MM.yyyy"); }
    m_name = new QLineEdit(user.name);
    m_phone = new QLineEdit;
    m_phone->setPlaceholderText("+7 (900) 000-00-00");
    f->addRow("Дата заезда:", m_from);
    f->addRow("Дата выезда:", m_to);
    f->addRow("ФИО гостя:", m_name);
    f->addRow("Телефон:", m_phone);

    auto *g2 = new QGroupBox("Дополнительные услуги");
    auto *sv = new QVBoxLayout(g2);
    m_breakfast = new QCheckBox(QString("Завтрак (%1 руб. в сутки)").arg(BREAKFAST_PER_NIGHT));
    m_transfer = new QCheckBox(QString("Трансфер из аэропорта (%1 руб.)").arg(TRANSFER));
    m_spa = new QCheckBox(QString("Посещение SPA (%1 руб.)").arg(SPA));
    sv->addWidget(m_breakfast); sv->addWidget(m_transfer); sv->addWidget(m_spa);

    m_total = new QLabel;
    m_total->setObjectName("total");
    auto *btn = new QPushButton("Подтвердить бронирование");

    v->addWidget(title);
    v->addWidget(room);
    v->addWidget(g1);
    v->addWidget(g2);
    v->addWidget(m_total);
    v->addWidget(btn);

    for (QCheckBox *c : {m_breakfast, m_transfer, m_spa})
        connect(c, &QCheckBox::toggled, this, &BookingDialog::updateTotal);
    connect(m_from, &QDateEdit::dateChanged, this, &BookingDialog::updateTotal);
    connect(m_to, &QDateEdit::dateChanged, this, &BookingDialog::updateTotal);
    connect(btn, &QPushButton::clicked, this, &BookingDialog::confirm);
    updateTotal();
}

int BookingDialog::calcTotal() const
{
    const int nights = qMax(0, int(m_from->date().daysTo(m_to->date())));
    int total = m_price * nights;
    if (m_breakfast->isChecked()) total += BREAKFAST_PER_NIGHT * nights;
    if (m_transfer->isChecked()) total += TRANSFER;
    if (m_spa->isChecked()) total += SPA;
    return total;
}

void BookingDialog::updateTotal()
{
    const int nights = qMax(0, int(m_from->date().daysTo(m_to->date())));
    m_total->setText(QString("Итого за %1 ноч.: %2 руб.").arg(nights).arg(calcTotal()));
}

void BookingDialog::fillDemo()
{
    m_phone->setText("+7 (900) 123-45-67");
    m_breakfast->setChecked(true);
    m_transfer->setChecked(true);
}

void BookingDialog::confirm()
{
    const QDate from = m_from->date(), to = m_to->date();
    QString digits = m_phone->text();
    digits.remove(QRegularExpression("\\D"));
    // проверка данных брони
    if (from >= to) {
        QMessageBox::warning(this, "Ошибка", "Дата выезда должна быть позже даты заезда");
        return;
    }
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите ФИО гостя");
        return;
    }
    if (digits.length() < 10) {
        QMessageBox::warning(this, "Ошибка", "Укажите корректный номер телефона");
        return;
    }
    if (!Database::isRoomFree(m_roomId, from, to)) {
        QMessageBox::warning(this, "Ошибка", "Номер уже занят на выбранные даты");
        return;
    }
    QStringList services;
    if (m_breakfast->isChecked()) services << "Завтрак";
    if (m_transfer->isChecked()) services << "Трансфер";
    if (m_spa->isChecked()) services << "SPA";

    QSqlQuery q;
    q.prepare("INSERT INTO bookings(user_id, room_id, guest_name, phone, date_from, date_to, services, total, status) "
              "VALUES(?, ?, ?, ?, ?, ?, ?, ?, 'Ожидает подтверждения')");
    q.addBindValue(m_user.id);
    q.addBindValue(m_roomId);
    q.addBindValue(m_name->text().trimmed());
    q.addBindValue(m_phone->text().trimmed());
    q.addBindValue(from.toString(Qt::ISODate));
    q.addBindValue(to.toString(Qt::ISODate));
    q.addBindValue(services.join(", "));
    q.addBindValue(calcTotal());
    if (q.exec()) {
        QMessageBox::information(this, "Бронирование",
                                 QString("Бронь № %1 оформлена.\nСумма: %2 руб.\nСтатус: ожидает подтверждения администратором.")
                                     .arg(q.lastInsertId().toInt()).arg(calcTotal()));
        accept();
    }
}
