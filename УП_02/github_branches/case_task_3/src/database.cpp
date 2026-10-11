#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QFile>
#include <QDebug>

namespace Database {

QString hashPassword(const QString &password)
{
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

static void seed()
{
    QSqlQuery q;
    // пользователи: администратор и тестовый гость
    q.prepare("INSERT INTO users(name, email, password, role) VALUES(?, ?, ?, ?)");
    const QList<QStringList> users = {
        {"Администратор", "admin@kosmos.ru", "admin123", "admin"},
        {"Иванов Иван Иванович", "guest@mail.ru", "123456", "guest"},
        {"Петрова Анна Сергеевна", "petrova@mail.ru", "123456", "guest"},
        {"Смирнов Олег Петрович", "smirnov@yandex.ru", "123456", "guest"}};
    for (const auto &u : users) {
        q.addBindValue(u[0]); q.addBindValue(u[1]);
        q.addBindValue(hashPassword(u[2])); q.addBindValue(u[3]);
        q.exec();
    }
    // номерной фонд
    q.prepare("INSERT INTO rooms(number, category, capacity, price, status) VALUES(?, ?, ?, ?, ?)");
    struct R { int n; const char *cat; int cap; int price; const char *st; };
    const R rooms[] = {
        {101, "Стандарт одноместный", 1, 3200, "Свободен"},
        {102, "Стандарт двухместный", 2, 4200, "Свободен"},
        {103, "Стандарт двухместный", 2, 4200, "На уборке"},
        {201, "Комфорт", 2, 5600, "Свободен"},
        {202, "Комфорт", 3, 6100, "Занят"},
        {203, "Семейный", 4, 7200, "Свободен"},
        {301, "Полулюкс", 2, 8300, "Свободен"},
        {302, "Люкс", 2, 9800, "В ремонте"},
        {303, "Люкс", 3, 11500, "Свободен"}};
    for (const R &r : rooms) {
        q.addBindValue(r.n); q.addBindValue(QString::fromUtf8(r.cat)); q.addBindValue(r.cap);
        q.addBindValue(r.price); q.addBindValue(QString::fromUtf8(r.st));
        q.exec();
    }
    // примеры бронирований
    const QDate d = QDate::currentDate();
    q.prepare("INSERT INTO bookings(user_id, room_id, guest_name, phone, date_from, date_to, services, total, status) "
              "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)");
    struct B { int user, room; const char *name, *phone; int from, nights; const char *serv; int total; const char *st; };
    const B bookings[] = {
        {2, 5, "Иванов Иван Иванович", "+7 (900) 123-45-67", -1, 3, "Завтрак", 20100, "Заселён"},
        {2, 4, "Иванов Иван Иванович", "+7 (900) 123-45-67", 10, 2, "", 11200, "Подтверждено"},
        {3, 7, "Петрова Анна Сергеевна", "+7 (951) 222-33-44", 3, 4, "Завтрак, SPA", 38100, "Ожидает подтверждения"},
        {4, 2, "Смирнов Олег Петрович", "+7 (920) 555-11-22", 5, 1, "Трансфер", 5700, "Ожидает подтверждения"},
        {3, 1, "Петрова Анна Сергеевна", "+7 (951) 222-33-44", -10, 2, "", 6400, "Выселен"}};
    for (const B &b : bookings) {
        q.addBindValue(b.user); q.addBindValue(b.room);
        q.addBindValue(QString::fromUtf8(b.name)); q.addBindValue(QString::fromUtf8(b.phone));
        q.addBindValue(d.addDays(b.from).toString(Qt::ISODate));
        q.addBindValue(d.addDays(b.from + b.nights).toString(Qt::ISODate));
        q.addBindValue(QString::fromUtf8(b.serv)); q.addBindValue(b.total); q.addBindValue(QString::fromUtf8(b.st));
        q.exec();
    }
}

bool init(const QString &path)
{
    const bool fresh = !QFile::exists(path);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning() << "Ошибка открытия БД:" << db.lastError().text();
        return false;
    }
    QSqlQuery q;
    q.exec("PRAGMA foreign_keys = ON");
    q.exec("CREATE TABLE IF NOT EXISTS users ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " name TEXT NOT NULL,"
           " email TEXT NOT NULL UNIQUE,"
           " password TEXT NOT NULL,"
           " role TEXT NOT NULL DEFAULT 'guest')");
    q.exec("CREATE TABLE IF NOT EXISTS rooms ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " number INTEGER NOT NULL UNIQUE,"
           " category TEXT NOT NULL,"
           " capacity INTEGER NOT NULL,"
           " price INTEGER NOT NULL,"
           " status TEXT NOT NULL DEFAULT 'Свободен')");
    q.exec("CREATE TABLE IF NOT EXISTS bookings ("
           " id INTEGER PRIMARY KEY AUTOINCREMENT,"
           " user_id INTEGER REFERENCES users(id),"
           " room_id INTEGER REFERENCES rooms(id),"
           " guest_name TEXT NOT NULL,"
           " phone TEXT,"
           " date_from TEXT NOT NULL,"
           " date_to TEXT NOT NULL,"
           " services TEXT,"
           " total INTEGER NOT NULL,"
           " status TEXT NOT NULL)");
    if (fresh)
        seed();
    return true;
}

bool login(const QString &email, const QString &password, User &user)
{
    QSqlQuery q;
    q.prepare("SELECT id, name, email, role FROM users WHERE email = ? AND password = ?");
    q.addBindValue(email.trimmed().toLower());
    q.addBindValue(hashPassword(password));
    if (q.exec() && q.next()) {
        user.id = q.value(0).toInt();
        user.name = q.value(1).toString();
        user.email = q.value(2).toString();
        user.role = q.value(3).toString();
        return true;
    }
    return false;
}

bool emailExists(const QString &email)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM users WHERE email = ?");
    q.addBindValue(email.trimmed().toLower());
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

bool registerGuest(const QString &name, const QString &email, const QString &password)
{
    QSqlQuery q;
    q.prepare("INSERT INTO users(name, email, password, role) VALUES(?, ?, ?, 'guest')");
    q.addBindValue(name.trimmed());
    q.addBindValue(email.trimmed().toLower());
    q.addBindValue(hashPassword(password));
    return q.exec();
}

bool isRoomFree(int roomId, const QDate &from, const QDate &to)
{
    // номер свободен, если нет активной брони, пересекающейся по датам
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM bookings WHERE room_id = ? "
              "AND status NOT IN ('Отменено', 'Выселен') "
              "AND date_from < ? AND date_to > ?");
    q.addBindValue(roomId);
    q.addBindValue(to.toString(Qt::ISODate));
    q.addBindValue(from.toString(Qt::ISODate));
    return q.exec() && q.next() && q.value(0).toInt() == 0;
}

} // namespace Database
