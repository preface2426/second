QT       += core gui network sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG   += c++17
TARGET = SecondHandBookClient
TEMPLATE = app
SOURCES += main.cpp \
           mainwindow.cpp \
           tcpclient.cpp\
           animatedbutton.cpp\
           paymanager.cpp\
           appealsubmitwindow.cpp\
           sellermsgwindow.cpp\
           buyhistorywindow.cpp

HEADERS += mainwindow.h \
           tcpclient.h\
           clickablelabel.h\
           animatedbutton.h\
           paymanager.h\
           appealsubmitwindow.h\
           sellermsgwindow.h\
           buyhistorywindow.h\


RESOURCES += resources.qrc\
             resources1.qrc
DISTFILES += \
    style.qss
