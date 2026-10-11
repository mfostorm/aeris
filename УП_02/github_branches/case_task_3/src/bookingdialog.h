// Окно оформления бронирования выбранного номера
#pragma once
#include <QDialog>
#include <QDate>
#include "database.h"
class QDateEdit;
class QLineEdit;
class QCheckBox;
class QLabel;

class BookingDialog : public QDialog
{
public:
    BookingDialog(const User &user, int roomId, const QDate &from, const QDate &to, QWidget *parent = nullptr);
    void fillDemo(); // заполнение для демонстрации

private:
    int calcTotal() const;
    void updateTotal();
    void confirm();

    User m_user;
    int m_roomId;
    int m_price = 0;
    QDateEdit *m_from, *m_to;
    QLineEdit *m_name, *m_phone;
    QCheckBox *m_breakfast, *m_transfer, *m_spa;
    QLabel *m_total;
};
