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
	   src/redismap.cpp

HEADERS += \
	   src/redismap.h
