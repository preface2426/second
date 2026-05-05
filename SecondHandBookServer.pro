QT += core network sql
CONFIG += console c++11
TARGET = SecondHandBookServer
TEMPLATE = app

SOURCES += \
    main.cpp \
    server.cpp \
    database.cpp\
    appealmanager.cpp\
    sellernotifymanager.cpp
HEADERS += \
    server.h \
    database.h\
    appealmanager.h\
    sellernotifymanager.h
