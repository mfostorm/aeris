// ИС «Космос» — графический интерфейс на C++ (Qt Widgets) для прототипов форм
// Автор: Паневина С.Е., группа СИд-023, УП.02, вариант 5
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QMessageBox>
#include "database.h"
#include "style.h"
#include "loginwindow.h"
#include "registerdialog.h"
#include "guestwindow.h"
#include "bookingdialog.h"
#include "adminwindow.h"

// Режим --screenshots <папка>: открывает все окна с демонстрационными данными
// и сохраняет их изображения (используется для иллюстраций отчёта)
static void makeScreenshots(const QString &dir)
{
    QDir().mkpath(dir);
    auto save = [&](QWidget *w, const QString &name) {
        w->show();
        for (int i = 0; i < 5; ++i) QApplication::processEvents();
        w->grab().save(dir + "/" + name + ".png");
    };
    User guest; Database::login("guest@mail.ru", "123456", guest);
    User admin; Database::login("admin@kosmos.ru", "admin123", admin);

    LoginWindow login;
    login.fill("guest@mail.ru", "123456");
    save(&login, "01_login");
    login.fill("guest@mail.ru", "111");
    login.tryLogin(false);
    save(&login, "02_login_error");
    login.hide();

    RegisterDialog reg;
    reg.fill("Козлова Мария", "kozlova@mail.ru", "qwerty1");
    save(&reg, "03_register");
    reg.hide();

    GuestWindow gw(guest);
    gw.selectFirstRoom();
    save(&gw, "04_guest_search");

    BookingDialog bd(guest, 4, QDate::currentDate().addDays(1), QDate::currentDate().addDays(4));
    bd.fillDemo();
    save(&bd, "05_booking");
    bd.hide();

    gw.showTab(1);
    save(&gw, "06_guest_bookings");
    gw.hide();

    AdminWindow aw(admin);
    aw.selectRow(0, 2);
    save(&aw, "07_admin_bookings");
    aw.showTab(1);
    aw.selectRow(1, 7);
    save(&aw, "08_admin_rooms");
    aw.showTab(2);
    save(&aw, "09_admin_users");
    aw.showTab(3);
    save(&aw, "10_admin_report");
    aw.hide();

    QMessageBox box(QMessageBox::Information, "Бронирование",
                    "Бронь № 6 оформлена.\nСумма: 20100 руб.\nСтатус: ожидает подтверждения администратором.");
    save(&box, "11_booking_done");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(appStyle());

    const QStringList args = app.arguments();
    const int si = args.indexOf("--screenshots");
    QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("hotel.db");
    if (si >= 0) {
        dbPath = QDir::temp().filePath("hotel_demo.db");
        QFile::remove(dbPath);
    }
    if (!Database::init(dbPath)) {
        QMessageBox::critical(nullptr, "Ошибка", "Не удалось открыть базу данных");
        return 1;
    }
    if (si >= 0) {
        makeScreenshots(si + 1 < args.size() ? args[si + 1] : "screenshots");
        return 0;
    }
    LoginWindow w;
    w.show();
    return app.exec();
}
