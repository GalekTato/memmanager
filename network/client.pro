CONFIG += c++20
CONFIG -= qt
LIBS += -lpthread
TARGET = memclient
TEMPLATE = app
INCLUDEPATH += ../include
HEADERS += Client.hpp
SOURCES += client_main.cpp Client.cpp