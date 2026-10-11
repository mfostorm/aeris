#include "guestwindow.h"
#include "bookingdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QTabWidget>
#include <QMessageBox>
#include <QSqlQuery>
#include <QStatusBar>

static QTableWidget *makeTable(const QStringList &headers)
{
    auto *t = new QTableWidget(0, headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->verticalHeader()->hide();
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
    t->setAlternatingRowColors(true);
    return t;
}

GuestWindow::GuestWindow(const User &user, QWidget *parent) : QMainWindow(parent), m_user(user)
{
    setWindowTitle("ИС «Космос» — личный кабинет гостя");
    resize(980, 600);

    auto *central = new QWidget;
    auto *v = new QVBoxLayout(central);
    v->setContentsMargins(16, 12, 16, 12);

    // шапка окна
    auto *top = new QHBoxLayout;
    auto *logo = new QLabel("Гостиница «Космос»");
    logo->setObjectName("header");
    auto *who = new QLabel("Гость: " + user.name);
    auto *logout = new QPushButton("Выйти");
    logout->setObjectName("secondary");
    top->addWidget(logo);
    top->addStretch();
    top->addWidget(who);
    top->addWidget(logout);
    v->addLayout(top);

    m_tabs = new QTabWidget;

    // вкладка «Поиск номеров»
    auto *searchTab = new QWidget;
    auto *sv = new QVBoxLayout(searchTab);
    auto *filters = new QHBoxLayout;
    m_from = new QDateEdit(QDate::currentDate().addDays(1));
    m_to = new QDateEdit(QDate::currentDate().addDays(4));
    for (QDateEdit *e : {m_from, m_to}) { e->setCalendarPopup(true); e->setDisplayFormat("dd.MM.yyyy"); }
    m_guests = new QSpinBox;
    m_guests->setRange(1, 6);
    m_guests->setValue(2);
    auto *btnSearch = new QPushButton("Найти");
    filters->addWidget(new QLabel("Заезд:"));  filters->addWidget(m_from);
    filters->addWidget(new QLabel("Выезд:"));  filters->addWidget(m_to);
    filters->addWidget(new QLabel("Гостей:")); filters->addWidget(m_guests);
    filters->addWidget(btnSearch);
    filters->addStretch();
    m_rooms = makeTable({"ID", "Номер", "Категория", "Мест", "Цена за ночь, руб.", "Стоимость за период, руб."});
    m_rooms->setColumnHidden(0, true);
    auto *btnBook = new QPushButton("Забронировать выбранный номер");
    sv->addLayout(filters);
    sv->addWidget(m_rooms);
    sv->addWidget(btnBook, 0, Qt::AlignRight);
    m_tabs->addTab(searchTab, "Поиск номеров");

    // вкладка «Мои бронирования»
    auto *bookTab = new QWidget;
    auto *bv = new QVBoxLayout(bookTab);
    m_bookings = makeTable({"№ брони", "Номер", "Категория", "Заезд", "Выезд", "Услуги", "Сумма, руб.", "Статус"});
    auto *btnCancel = new QPushButton("Отменить бронь");
    btnCancel->setObjectName("danger");
    bv->addWidget(m_bookings);
    bv->addWidget(btnCancel, 0, Qt::AlignRight);
    m_tabs->addTab(bookTab, "Мои бронирования");

    v->addWidget(m_tabs);
    setCentralWidget(central);
    statusBar()->showMessage("Выберите даты проживания и нажмите «Найти»");

    connect(btnSearch, &QPushButton::clicked, this, &GuestWindow::search);
    connect(btnBook, &QPushButton::clicked, this, &GuestWindow::bookSelected);
    connect(btnCancel, &QPushButton::clicked, this, &GuestWindow::cancelSelected);
    connect(logout, &QPushButton::clicked, this, &QWidget::close);

    search();
    loadBookings();
}

void GuestWindow::search()
{
    const QDate from = m_from->date(), to = m_to->date();
    if (from >= to) {
        QMessageBox::warning(this, "Поиск", "Дата выезда должна быть позже даты заезда");
        return;
    }
    if (from < QDate::currentDate()) {
        QMessageBox::warning(this, "Поиск", "Дата заезда не может быть в прошлом");
        return;
    }
    const int nights = from.daysTo(to);
    m_rooms->setRowCount(0);
    QSqlQuery q;
    q.prepare("SELECT id, number, category, capacity, price FROM rooms "
              "WHERE capacity >= ? AND status <> 'В ремонте' ORDER BY number");
    q.addBindValue(m_guests->value());
    q.exec();
    while (q.next()) {
        const int id = q.value(0).toInt();
        if (!Database::isRoomFree(id, from, to))
            continue;
        const int row = m_rooms->rowCount();
        m_rooms->insertRow(row);
        const QStringList cells = {q.value(0).toString(), q.value(1).toString(), q.value(2).toString(),
                                   q.value(3).toString(), q.value(4).toString(),
                                   QString::number(q.value(4).toInt() * nights)};
        for (int c = 0; c < cells.size(); ++c)
            m_rooms->setItem(row, c, new QTableWidgetItem(cells[c]));
    }
    statusBar()->showMessage(QString("Найдено свободных номеров: %1 (ночей: %2)").arg(m_rooms->rowCount()).arg(nights));
}

void GuestWindow::loadBookings()
{
    m_bookings->setRowCount(0);
    QSqlQuery q;
    q.prepare("SELECT b.id, r.number, r.category, b.date_from, b.date_to, b.services, b.total, b.status "
              "FROM bookings b JOIN rooms r ON r.id = b.room_id WHERE b.user_id = ? ORDER BY b.date_from DESC");
    q.addBindValue(m_user.id);
    q.exec();
    while (q.next()) {
        const int row = m_bookings->rowCount();
        m_bookings->insertRow(row);
        for (int c = 0; c < 8; ++c) {
            QString text = q.value(c).toString();
            if (c == 3 || c == 4)
                text = QDate::fromString(text, Qt::ISODate).toString("dd.MM.yyyy");
            if (c == 5 && text.isEmpty())
                text = "—";
            m_bookings->setItem(row, c, new QTableWidgetItem(text));
        }
    }
}

void GuestWindow::showTab(int index) { m_tabs->setCurrentIndex(index); }
void GuestWindow::selectFirstRoom() { if (m_rooms->rowCount() > 0) m_rooms->selectRow(0); }

void GuestWindow::bookSelected()
{
    const int row = m_rooms->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Бронирование", "Выберите номер в списке");
        return;
    }
    BookingDialog dlg(m_user, m_rooms->item(row, 0)->text().toInt(), m_from->date(), m_to->date(), this);
    if (dlg.exec() == QDialog::Accepted) {
        search();
        loadBookings();
        m_tabs->setCurrentIndex(1);
    }
}

void GuestWindow::cancelSelected()
{
    const int row = m_bookings->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Отмена", "Выберите бронь в списке");
        return;
    }
    const QString status = m_bookings->item(row, 7)->text();
    if (status == "Заселён" || status == "Выселен" || status == "Отменено") {
        QMessageBox::warning(this, "Отмена", "Эту бронь отменить нельзя (статус: " + status + ")");
        return;
    }
    if (QMessageBox::question(this, "Отмена брони", "Отменить выбранную бронь?") != QMessageBox::Yes)
        return;
    QSqlQuery q;
    q.prepare("UPDATE bookings SET status = 'Отменено' WHERE id = ? AND user_id = ?");
    q.addBindValue(m_bookings->item(row, 0)->text().toInt());
    q.addBindValue(m_user.id);
    q.exec();
    loadBookings();
}
