QT       += core network
CONFIG   += c++11

SOURCES += $$PWD/src/redisserver.cpp \
           $$PWD/src/redislistpoller.cpp

HEADERS += $$PWD/include/redust/redishash.h \
           $$PWD/include/redust/redisserver.h \
           $$PWD/include/redust/typeserializer.h \
           $$PWD/include/redust/redislistpoller.h

# Additional helper headers for easy access
HEADERS += $$PWD/include/redust/RedisHash \
           $$PWD/include/redust/RedisServer \
           $$PWD/include/redust/TypeSerializer

INCLUDEPATH += $$PWD/include

# protobuffer support
defined(REDUST_SUPPORT_PROTOBUF,var) {
    DEFINES += "REDISMAP_SUPPORT_PROTOBUF"
    LIBS += -lprotobuf
}
