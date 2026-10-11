// Панель администратора: бронирования, номерной фонд, пользователи, отчёт
#pragma once
#include <QMainWindow>
#include "database.h"
class QTabWidget;
class QTableView;
class QSqlQueryModel;
class QSqlTableModel;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QLabel;

class AdminWindow : public QMainWindow
{
public:
    explicit AdminWindow(const User &user, QWidget *parent = nullptr);
    void showTab(int index);
    void selectRow(int tab, int row);

private:
    QWidget *createBookingsTab();
    QWidget *createRoomsTab();
    QWidget *createUsersTab();
    QWidget *createReportTab();
    void loadBookings();
    void setBookingStatus(const QString &status);
    void addRoom();
    void deleteRoom();
    void changeRoomStatus();
    void addUser();
    void deleteUser();
    void buildReport();

    User m_user;
    QTabWidget *m_tabs;
    // бронирования
    QTableView *m_bookView;
    QSqlQueryModel *m_bookModel;
    QComboBox *m_bookFilter;
    // номера
    QTableView *m_roomView;
    QSqlTableModel *m_roomModel;
    QSpinBox *m_roomNumber, *m_roomCap, *m_roomPrice;
    QComboBox *m_roomCat, *m_roomStatus;
    // пользователи
    QTableView *m_userView;
    QSqlTableModel *m_userModel;
    QLineEdit *m_userName, *m_userEmail, *m_userPass;
    QComboBox *m_userRole;
    // отчёт
    QLabel *m_report;
};
