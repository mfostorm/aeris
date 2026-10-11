// Личный кабинет гостя: поиск номеров и список бронирований
#pragma once
#include <QMainWindow>
#include "database.h"
class QDateEdit;
class QSpinBox;
class QTableWidget;
class QTabWidget;

class GuestWindow : public QMainWindow
{
public:
    explicit GuestWindow(const User &user, QWidget *parent = nullptr);
    void search();            // поиск свободных номеров
    void loadBookings();      // загрузка бронирований гостя
    void showTab(int index);
    void selectFirstRoom();

private:
    void bookSelected();
    void cancelSelected();

    User m_user;
    QTabWidget *m_tabs;
    QDateEdit *m_from, *m_to;
    QSpinBox *m_guests;
    QTableWidget *m_rooms, *m_bookings;
};
