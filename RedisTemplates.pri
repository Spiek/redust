#-------------------------------------------------
#
# Project created by QtCreator 2014-11-14T23:07:36
#
#-------------------------------------------------

QT       += core network
CONFIG   += c++11

SOURCES += $$PWD/src/redismapconnectionmanager.cpp \
           $$PWD/src/redisinterface.cpp

HEADERS += $$PWD/src/redishash.h \
           $$PWD/src/redismapconnectionmanager.h \
           $$PWD/src/redisvalue.h \
           $$PWD/src/redisinterface.h
INCLUDEPATH += $$PWD/src

# link against protobuf lib
LIBS += -lprotobuf
