#-------------------------------------------------
#
# Project created by QtCreator 2014-11-14T23:07:36
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET    = RedisMap
CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += c++11

TEMPLATE = app


SOURCES += src/redismapconnectionmanager.cpp \
           src/redismapprivate.cpp

HEADERS += src/redishash.h \
           src/redismapconnectionmanager.h \
           src/redisvalue.h \
           src/redismapprivate.h
INCLUDEPATH += src

# Tests
SOURCES += test/main.cpp
SOURCES += test/test.cpp
HEADERS += test/test.h
INCLUDEPATH += test

# Google protobuffer Test messages
HEADERS += test/test.pb.h
SOURCES += test/test.pb.cc

# Features
DEFINES += "REDISMAP_SUPPORT_PROTOBUF"

# link against protobuf lib
LIBS += -lprotobuf
