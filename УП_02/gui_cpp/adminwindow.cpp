#include "adminwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableView>
#include <QHeaderView>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QMessageBox>
#include <QStatusBar>
#include <QDate>

static const QStringList ROOM_STATUSES = {"Свободен", "Забронирован", "Занят", "На уборке", "В ремонте"};
static const QStringList CATEGORIES = {"Стандарт одноместный", "Стандарт двухместный", "Комфорт",
                                       "Семейный", "Полулюкс", "Люкс"};

static void setupView(QTableView *v)
{
    v->setSelectionBehavior(QAbstractItemView::SelectRows);
    v->setSelectionMode(QAbstractItemView::SingleSelection);
    v->verticalHeader()->hide();
    v->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    v->horizontalHeader()->setStretchLastSection(true);
    v->setAlternatingRowColors(true);
}

AdminWindow::AdminWindow(const User &user, QWidget *parent) : QMainWindow(parent), m_user(user)
{
    setWindowTitle("ИС «Космос» — панель администратора");
    resize(1100, 640);

    auto *central = new QWidget;
    auto *v = new QVBoxLayout(central);
    v->setContentsMargins(16, 12, 16, 12);
    auto *top = new QHBoxLayout;
    auto *logo = new QLabel("Гостиница «Космос» — панель администратора");
    logo->setObjectName("header");
    auto *logout = new QPushButton("Выйти");
    logout->setObjectName("secondary");
    top->addWidget(logo);
    top->addStretch();
    top->addWidget(new QLabel(user.email));
    top->addWidget(logout);
    v->addLayout(top);

    m_tabs = new QTabWidget;
    m_tabs->addTab(createBookingsTab(), "Бронирования");
    m_tabs->addTab(createRoomsTab(), "Номерной фонд");
    m_tabs->addTab(createUsersTab(), "Пользователи");
    m_tabs->addTab(createReportTab(), "Отчёт");
    v->addWidget(m_tabs);
    setCentralWidget(central);
    statusBar()->showMessage("База данных: hotel.db (SQLite)");

    connect(logout, &QPushButton::clicked, this, &QWidget::close);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int i) {
        if (i == 0) loadBookings();
        if (i == 1) m_roomModel->select();
        if (i == 3) buildReport();
    });
}

void AdminWindow::showTab(int index) { m_tabs->setCurrentIndex(index); }

void AdminWindow::selectRow(int tab, int row)
{
    QTableView *views[] = {m_bookView, m_roomView, m_userView};
    if (tab >= 0 && tab < 3) views[tab]->selectRow(row);
}

// ---------------- вкладка «Бронирования»
QWidget *AdminWindow::createBookingsTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    auto *h = new QHBoxLayout;
    m_bookFilter = new QComboBox;
    m_bookFilter->addItems({"Все", "Ожидает подтверждения", "Подтверждено", "Заселён", "Выселен", "Отменено"});
    h->addWidget(new QLabel("Статус:"));
    h->addWidget(m_bookFilter);
    h->addStretch();
    v->addLayout(h);

    m_bookModel = new QSqlQueryModel(this);
    m_bookView = new QTableView;
    m_bookView->setModel(m_bookModel);
    setupView(m_bookView);
    v->addWidget(m_bookView);

    auto *btns = new QHBoxLayout;
    auto *bConfirm = new QPushButton("Подтвердить");
    auto *bIn = new QPushButton("Заселить");
    auto *bOut = new QPushButton("Выселить");
    auto *bCancel = new QPushButton("Отменить бронь");
    bCancel->setObjectName("danger");
    btns->addStretch();
    for (QPushButton *b : {bConfirm, bIn, bOut, bCancel}) btns->addWidget(b);
    v->addLayout(btns);

    connect(m_bookFilter, &QComboBox::currentIndexChanged, this, &AdminWindow::loadBookings);
    connect(bConfirm, &QPushButton::clicked, this, [this] { setBookingStatus("Подтверждено"); });
    connect(bIn, &QPushButton::clicked, this, [this] { setBookingStatus("Заселён"); });
    connect(bOut, &QPushButton::clicked, this, [this] { setBookingStatus("Выселен"); });
    connect(bCancel, &QPushButton::clicked, this, [this] { setBookingStatus("Отменено"); });
    loadBookings();
    return w;
}

void AdminWindow::loadBookings()
{
    QString sql = "SELECT b.id AS 'Бронь', b.guest_name AS 'Гость', b.phone AS 'Телефон', r.number AS 'Номер', "
                  "strftime('%d.%m.%Y', b.date_from) AS 'Заезд', strftime('%d.%m.%Y', b.date_to) AS 'Выезд', "
                  "b.total AS 'Сумма, руб.', b.status AS 'Статус' "
                  "FROM bookings b JOIN rooms r ON r.id = b.room_id ";
    QSqlQuery q;
    if (m_bookFilter->currentIndex() > 0) {
        q.prepare(sql + "WHERE b.status = ? ORDER BY b.date_from");
        q.addBindValue(m_bookFilter->currentText());
    } else {
        q.prepare(sql + "ORDER BY b.date_from");
    }
    q.exec();
    m_bookModel->setQuery(std::move(q));
}

void AdminWindow::setBookingStatus(const QString &status)
{
    const QModelIndex idx = m_bookView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "Бронирования", "Выберите бронь в таблице");
        return;
    }
    const int id = m_bookModel->record(idx.row()).value(0).toInt();
    const QString current = m_bookModel->record(idx.row()).value(7).toString();
    // проверка допустимости перехода между статусами
    if ((status == "Заселён" && current != "Подтверждено") ||
        (status == "Выселен" && current != "Заселён") ||
        (status == "Подтверждено" && current != "Ожидает подтверждения") ||
        (status == "Отменено" && (current == "Заселён" || current == "Выселен"))) {
        QMessageBox::warning(this, "Бронирования",
                             QString("Нельзя изменить статус «%1» на «%2»").arg(current, status));
        return;
    }
    QSqlQuery q;
    q.prepare("UPDATE bookings SET status = ? WHERE id = ?");
    q.addBindValue(status);
    q.addBindValue(id);
    q.exec();
    // статус номера меняется вместе со статусом брони
    QString roomStatus;
    if (status == "Заселён") roomStatus = "Занят";
    if (status == "Выселен") roomStatus = "На уборке";
    if (!roomStatus.isEmpty()) {
        q.prepare("UPDATE rooms SET status = ? WHERE id = (SELECT room_id FROM bookings WHERE id = ?)");
        q.addBindValue(roomStatus);
        q.addBindValue(id);
        q.exec();
    }
    loadBookings();
    statusBar()->showMessage(QString("Бронь № %1: статус изменён на «%2»").arg(id).arg(status));
}

// ---------------- вкладка «Номерной фонд»
QWidget *AdminWindow::createRoomsTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    m_roomModel = new QSqlTableModel(this);
    m_roomModel->setTable("rooms");
    m_roomModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_roomModel->setSort(1, Qt::AscendingOrder);
    m_roomModel->select();
    const QStringList headers = {"ID", "Номер", "Категория", "Мест", "Цена за ночь, руб.", "Статус"};
    for (int i = 0; i < headers.size(); ++i)
        m_roomModel->setHeaderData(i, Qt::Horizontal, headers[i]);
    m_roomView = new QTableView;
    m_roomView->setModel(m_roomModel);
    m_roomView->setColumnHidden(0, true);
    setupView(m_roomView);
    v->addWidget(m_roomView);

    auto *row = new QHBoxLayout;
    // добавление номера
    auto *gAdd = new QGroupBox("Добавить номер");
    auto *ga = new QGridLayout(gAdd);
    m_roomNumber = new QSpinBox; m_roomNumber->setRange(100, 999); m_roomNumber->setValue(304);
    m_roomCat = new QComboBox; m_roomCat->addItems(CATEGORIES);
    m_roomCap = new QSpinBox; m_roomCap->setRange(1, 6); m_roomCap->setValue(2);
    m_roomPrice = new QSpinBox; m_roomPrice->setRange(500, 100000); m_roomPrice->setSingleStep(100); m_roomPrice->setValue(5000);
    auto *bAdd = new QPushButton("Добавить");
    ga->addWidget(new QLabel("Номер"), 0, 0);    ga->addWidget(m_roomNumber, 1, 0);
    ga->addWidget(new QLabel("Категория"), 0, 1); ga->addWidget(m_roomCat, 1, 1);
    ga->addWidget(new QLabel("Мест"), 0, 2);     ga->addWidget(m_roomCap, 1, 2);
    ga->addWidget(new QLabel("Цена"), 0, 3);     ga->addWidget(m_roomPrice, 1, 3);
    ga->addWidget(bAdd, 1, 4);
    // изменение статуса и удаление
    auto *gSt = new QGroupBox("Выбранный номер");
    auto *gs = new QHBoxLayout(gSt);
    m_roomStatus = new QComboBox; m_roomStatus->addItems(ROOM_STATUSES);
    auto *bSt = new QPushButton("Изменить статус");
    auto *bDel = new QPushButton("Удалить");
    bDel->setObjectName("danger");
    gs->addWidget(m_roomStatus); gs->addWidget(bSt); gs->addWidget(bDel);
    row->addWidget(gAdd, 3);
    row->addWidget(gSt, 2);
    v->addLayout(row);

    connect(bAdd, &QPushButton::clicked, this, &AdminWindow::addRoom);
    connect(bSt, &QPushButton::clicked, this, &AdminWindow::changeRoomStatus);
    connect(bDel, &QPushButton::clicked, this, &AdminWindow::deleteRoom);
    return w;
}

void AdminWindow::addRoom()
{
    QSqlQuery q;
    q.prepare("INSERT INTO rooms(number, category, capacity, price, status) VALUES(?, ?, ?, ?, 'Свободен')");
    q.addBindValue(m_roomNumber->value());
    q.addBindValue(m_roomCat->currentText());
    q.addBindValue(m_roomCap->value());
    q.addBindValue(m_roomPrice->value());
    if (!q.exec()) {
        QMessageBox::warning(this, "Номерной фонд", "Номер с таким числом уже существует");
        return;
    }
    m_roomModel->select();
    statusBar()->showMessage(QString("Номер %1 добавлен").arg(m_roomNumber->value()));
}

void AdminWindow::changeRoomStatus()
{
    const QModelIndex idx = m_roomView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "Номерной фонд", "Выберите номер в таблице");
        return;
    }
    m_roomModel->setData(m_roomModel->index(idx.row(), 5), m_roomStatus->currentText());
    m_roomModel->select();
}

void AdminWindow::deleteRoom()
{
    const QModelIndex idx = m_roomView->currentIndex();
    if (!idx.isValid())
        return;
    const int roomId = m_roomModel->record(idx.row()).value("id").toInt();
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM bookings WHERE room_id = ? AND status IN ('Ожидает подтверждения', 'Подтверждено', 'Заселён')");
    q.addBindValue(roomId);
    if (q.exec() && q.next() && q.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Номерной фонд", "Нельзя удалить номер с активными бронями");
        return;
    }
    if (QMessageBox::question(this, "Номерной фонд", "Удалить выбранный номер?") == QMessageBox::Yes) {
        m_roomModel->removeRow(idx.row());
        m_roomModel->select();
    }
}

// ---------------- вкладка «Пользователи»
QWidget *AdminWindow::createUsersTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    m_userModel = new QSqlTableModel(this);
    m_userModel->setTable("users");
    m_userModel->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_userModel->select();
    const QStringList headers = {"ID", "ФИО", "Email", "Пароль (хеш)", "Роль"};
    for (int i = 0; i < headers.size(); ++i)
        m_userModel->setHeaderData(i, Qt::Horizontal, headers[i]);
    m_userView = new QTableView;
    m_userView->setModel(m_userModel);
    m_userView->setColumnHidden(3, true); // хеш пароля не показываем
    setupView(m_userView);
    v->addWidget(m_userView);

    auto *g = new QGroupBox("Добавить пользователя");
    auto *h = new QHBoxLayout(g);
    m_userName = new QLineEdit;  m_userName->setPlaceholderText("ФИО");
    m_userEmail = new QLineEdit; m_userEmail->setPlaceholderText("Email");
    m_userPass = new QLineEdit;  m_userPass->setPlaceholderText("Пароль"); m_userPass->setEchoMode(QLineEdit::Password);
    m_userRole = new QComboBox;  m_userRole->addItems({"guest", "admin"});
    auto *bAdd = new QPushButton("Добавить");
    auto *bDel = new QPushButton("Удалить выбранного");
    bDel->setObjectName("danger");
    for (QWidget *x : std::initializer_list<QWidget *>{m_userName, m_userEmail, m_userPass, m_userRole, bAdd, bDel})
        h->addWidget(x);
    v->addWidget(g);

    connect(bAdd, &QPushButton::clicked, this, &AdminWindow::addUser);
    connect(bDel, &QPushButton::clicked, this, &AdminWindow::deleteUser);
    return w;
}

void AdminWindow::addUser()
{
    if (m_userName->text().trimmed().isEmpty() || !m_userEmail->text().contains('@') || m_userPass->text().length() < 6) {
        QMessageBox::warning(this, "Пользователи", "Заполните ФИО, корректный email и пароль (от 6 символов)");
        return;
    }
    if (Database::emailExists(m_userEmail->text())) {
        QMessageBox::warning(this, "Пользователи", "Такой email уже зарегистрирован");
        return;
    }
    QSqlQuery q;
    q.prepare("INSERT INTO users(name, email, password, role) VALUES(?, ?, ?, ?)");
    q.addBindValue(m_userName->text().trimmed());
    q.addBindValue(m_userEmail->text().trimmed().toLower());
    q.addBindValue(Database::hashPassword(m_userPass->text()));
    q.addBindValue(m_userRole->currentText());
    q.exec();
    m_userModel->select();
    m_userName->clear(); m_userEmail->clear(); m_userPass->clear();
}

void AdminWindow::deleteUser()
{
    const QModelIndex idx = m_userView->currentIndex();
    if (!idx.isValid())
        return;
    if (m_userModel->record(idx.row()).value("id").toInt() == m_user.id) {
        QMessageBox::warning(this, "Пользователи", "Нельзя удалить собственную учётную запись");
        return;
    }
    if (QMessageBox::question(this, "Пользователи", "Удалить выбранного пользователя?") == QMessageBox::Yes) {
        m_userModel->removeRow(idx.row());
        m_userModel->select();
    }
}

// ---------------- вкладка «Отчёт»
QWidget *AdminWindow::createReportTab()
{
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    auto *title = new QLabel("Сводный отчёт по гостинице");
    title->setObjectName("header");
    m_report = new QLabel;
    m_report->setTextFormat(Qt::RichText);
    m_report->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    auto *btn = new QPushButton("Сформировать отчёт");
    v->addWidget(title);
    v->addWidget(m_report, 1);
    v->addWidget(btn, 0, Qt::AlignLeft);
    connect(btn, &QPushButton::clicked, this, &AdminWindow::buildReport);
    buildReport();
    return w;
}

void AdminWindow::buildReport()
{
    auto scalar = [](const QString &sql) {
        QSqlQuery q(sql);
        return q.next() ? q.value(0).toInt() : 0;
    };
    const int rooms = scalar("SELECT COUNT(*) FROM rooms");
    const int busy = scalar("SELECT COUNT(*) FROM rooms WHERE status = 'Занят'");
    const int repair = scalar("SELECT COUNT(*) FROM rooms WHERE status = 'В ремонте'");
    const int active = scalar("SELECT COUNT(*) FROM bookings WHERE status IN ('Ожидает подтверждения', 'Подтверждено', 'Заселён')");
    const int waiting = scalar("SELECT COUNT(*) FROM bookings WHERE status = 'Ожидает подтверждения'");
    const int revenue = scalar("SELECT COALESCE(SUM(total), 0) FROM bookings WHERE status IN ('Подтверждено', 'Заселён', 'Выселен')");
    const int users = scalar("SELECT COUNT(*) FROM users WHERE role = 'guest'");
    const double load = rooms ? 100.0 * busy / rooms : 0;

    m_report->setText(QString(
        "<table cellpadding='7' style='font-size:11pt'>"
        "<tr><td>Дата формирования:</td><td><b>%1</b></td></tr>"
        "<tr><td>Всего номеров:</td><td><b>%2</b></td></tr>"
        "<tr><td>Занято номеров:</td><td><b>%3</b></td></tr>"
        "<tr><td>Номеров в ремонте:</td><td><b>%4</b></td></tr>"
        "<tr><td>Текущая загрузка:</td><td><b>%5 %</b></td></tr>"
        "<tr><td>Активных бронирований:</td><td><b>%6</b> (ожидают подтверждения: %7)</td></tr>"
        "<tr><td>Выручка по подтверждённым броням:</td><td><b>%8 руб.</b></td></tr>"
        "<tr><td>Зарегистрировано гостей:</td><td><b>%9</b></td></tr>"
        "</table>")
        .arg(QDate::currentDate().toString("dd.MM.yyyy"))
        .arg(rooms).arg(busy).arg(repair).arg(load, 0, 'f', 1)
        .arg(active).arg(waiting).arg(revenue).arg(users));
}
