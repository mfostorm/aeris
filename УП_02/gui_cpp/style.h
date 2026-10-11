// Общий стиль интерфейса ИС «Космос» (цвета взяты из веб-прототипа)
#pragma once
#include <QString>

inline QString appStyle()
{
    return R"(
        QWidget { font-family: "DejaVu Sans", "Segoe UI"; font-size: 10pt; color: #1f2937; }
        QMainWindow, QDialog { background: #f3f5f9; }
        QFrame#card { background: #ffffff; border: 1px solid #e3e7ee; border-radius: 10px; }
        QLabel#title { font-size: 17pt; font-weight: bold; color: #1f2937; }
        QLabel#subtitle { color: #6b7280; }
        QLabel#header { font-size: 13pt; font-weight: bold; color: #1e3a8a; }
        QLabel#total { font-size: 12pt; font-weight: bold; color: #1d4ed8; }
        QLineEdit, QDateEdit, QSpinBox, QComboBox {
            background: #ffffff; border: 1px solid #cfd6e1; border-radius: 6px; padding: 5px 8px; min-height: 20px; }
        QLineEdit:focus, QDateEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #4a8be8; }
        QPushButton { background: #4a8be8; color: white; border: none; border-radius: 6px; padding: 7px 16px; font-weight: bold; }
        QPushButton:hover { background: #3a78d4; }
        QPushButton#secondary { background: #e8eef8; color: #1d4ed8; }
        QPushButton#danger { background: #e5534b; }
        QPushButton#link { background: transparent; color: #2563eb; padding: 2px; font-weight: normal; }
        QTabWidget::pane { border: 1px solid #e3e7ee; background: #ffffff; border-radius: 6px; }
        QTabBar::tab { background: #e8eef8; padding: 8px 18px; margin-right: 2px; border-top-left-radius: 6px; border-top-right-radius: 6px; }
        QTabBar::tab:selected { background: #4a8be8; color: white; }
        QTableView { background: #ffffff; gridline-color: #e5e7eb; selection-background-color: #dbe8fc; selection-color: #111827; }
        QHeaderView::section { background: #eef2f8; padding: 6px; border: none; border-right: 1px solid #dde3ec; font-weight: bold; }
        QGroupBox { font-weight: bold; border: 1px solid #e3e7ee; border-radius: 8px; margin-top: 12px; padding: 10px; background: #ffffff; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
        QStatusBar { background: #e8eef8; }
    )";
}
