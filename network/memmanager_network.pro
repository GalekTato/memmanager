CONFIG += c++20
CONFIG -= qt
LIBS += -lpthread

TARGET = memserver
TEMPLATE = app

INCLUDEPATH += ../include

HEADERS += Server.hpp
SOURCES += server_main.cpp Server.cpp ../include/Dispatcher.cpp
