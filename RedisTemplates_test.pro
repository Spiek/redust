QT += testlib core network
TEMPLATE = app
TARGET = RedisTemplates.test
INCLUDEPATH += src/test

# Features
DEFINES += "REDISMAP_SUPPORT_PROTOBUF"

# Google protobuffer Test messages
HEADERS += test/test.pb.h
SOURCES += test/test.pb.cc

# Test cases
SOURCES += test/cases/testredishash.cpp

# Redis Connection properties
DEFINES += REDIS_SERVER=\\\"127.0.0.1\\\"
DEFINES += REDIS_SERVER_PORT=6379

# include redis templates
include(RedisTemplates.pri)
