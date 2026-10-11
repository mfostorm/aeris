cmake_minimum_required(VERSION 3.16)
project(HotelKosmos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(HotelKosmos
    main.cpp
    database.h database.cpp
    style.h
    loginwindow.h loginwindow.cpp
    registerdialog.h registerdialog.cpp
    guestwindow.h guestwindow.cpp
    bookingdialog.h bookingdialog.cpp
    adminwindow.h adminwindow.cpp
)
target_link_libraries(HotelKosmos PRIVATE Qt6::Widgets Qt6::Sql)
