QT       += core network
CONFIG   += c++11

SOURCES += $$PWD/src/redisconnectionmanager.cpp \
           $$PWD/src/redisinterface.cpp \
           $$PWD/src/redislistpoller.cpp

HEADERS += $$PWD/include/redust/redishash.h \
           $$PWD/include/redust/redisconnectionmanager.h \
           $$PWD/include/redust/typeserializer.h \
           $$PWD/include/redust/redisinterface.h \
           $$PWD/include/redust/redislistpoller.h

# Additional helper headers for easy access
HEADERS += $$PWD/include/redust/RedisHash \
           $$PWD/include/redust/RedisConnectionManager \
           $$PWD/include/redust/TypeSerializer \
           $$PWD/include/redust/RedisInterface

INCLUDEPATH += $$PWD/include

# link against protobuf lib
LIBS += -lprotobuf
