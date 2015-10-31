#-------------------------------------------------
#
# Project created by QtCreator 2014-11-14T23:07:36
#
#-------------------------------------------------

QT       += core network
QT       -= gui
CONFIG   += c++11

SOURCES += src/redismapconnectionmanager.cpp \
           src/redisinterface.cpp

HEADERS += src/redishash.h \
           src/redismapconnectionmanager.h \
           src/redisvalue.h \
           src/redisinterface.h
INCLUDEPATH += src

# link against protobuf lib
LIBS += -lprotobuf
