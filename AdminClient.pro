QT += widgets network
CONFIG += c++11
TEMPLATE = app
TARGET = AdminClient

SOURCES += \
    main.cpp \
    adminwindow.cpp \
    tcpclient.cpp\
    appealreviewwindow.cpp\

HEADERS += \
    adminwindow.h \
    tcpclient.h\
    appealreviewwindow.h

RESOURCES += resources.qrc\
