QT       += core network
CONFIG   += c++11

SOURCES += $$PWD/src/redisconnectionmanager.cpp \
           $$PWD/src/redisinterface.cpp

HEADERS += $$PWD/include/recotec/redishash.h \
           $$PWD/include/recotec/redisconnectionmanager.h \
           $$PWD/include/recotec/typeserializer.h \
           $$PWD/include/recotec/redisinterface.h
INCLUDEPATH += $$PWD/include

# link against protobuf lib
LIBS += -lprotobuf
