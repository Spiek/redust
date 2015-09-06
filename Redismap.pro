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


SOURCES += src/main.cpp \
	   src/redismapconnectionmanager.cpp \
	   src/test.cpp

HEADERS += \
	   src/redismap.h \
	   src/redismapconnectionmanager.h \
	   src/test.h

# Settings
DEFINES += "REDISMAP_INIT_QUERY_BUFFER_CACHE_SIZE=1048576"
DEFINES += "REDISMAP_MAX_QUERY_BUFFER_CACHE_SIZE=10485760"

# Features
DEFINES += "REDISMAP_SUPPORT_PROTOBUF"
