// Работа с базой данных SQLite: создание таблиц и начальное заполнение
#pragma once
#include <QString>
#include <QDate>

struct User {
    int id = 0;
    QString name;
    QString email;
    QString role;   // "admin" или "guest"
};

namespace Database {
    bool init(const QString &path);                 // открыть БД и создать таблицы
    QString hashPassword(const QString &password);  // SHA-256 хеш пароля
    bool login(const QString &email, const QString &password, User &user);
    bool emailExists(const QString &email);
    bool registerGuest(const QString &name, const QString &email, const QString &password);
    bool isRoomFree(int roomId, const QDate &from, const QDate &to);
}
