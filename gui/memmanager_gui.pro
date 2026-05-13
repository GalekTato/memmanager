QT       += core gui widgets
CONFIG   += c++20
TARGET    = memmanager_gui
TEMPLATE  = app

INCLUDEPATH += ../include

HEADERS += MainWindow.hpp

SOURCES += main_gui.cpp \
           MainWindow.cpp \
           ../include/Dispatcher.cpp

# Silence deprecation warnings from Qt internals
DEFINES += QT_DEPRECATED_WARNINGS
