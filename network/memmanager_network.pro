CONFIG += c++20
CONFIG -= qt
LIBS += -lpthread

TEMPLATE = subdirs
SUBDIRS = memserver memclient memmonitor

memserver.file = server.pro
memclient.file = client.pro
memmonitor.file = monitor.pro