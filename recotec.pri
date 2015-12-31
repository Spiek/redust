QT       += core network
CONFIG   += c++11

SOURCES += $$PWD/src/redisconnectionmanager.cpp \
           $$PWD/src/redisinterface.cpp

HEADERS += $$PWD/include/recotec/redishash.h \
           $$PWD/include/recotec/redisconnectionmanager.h \
           $$PWD/include/recotec/typeserializer.h \
           $$PWD/include/recotec/redisinterface.h

# Additional helper headers for easy access
HEADERS += $$PWD/include/recotec/RedisHash \
           $$PWD/include/recotec/RedisConnectionManager \
           $$PWD/include/recotec/TypeSerializer \
           $$PWD/include/recotec/RedisInterface

INCLUDEPATH += $$PWD/include

# link against protobuf lib
LIBS += -lprotobuf
