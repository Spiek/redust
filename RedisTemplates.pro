
TARGET   = redistemplates
TEMPLATE = lib
CONFIG   += console
CONFIG   -= app_bundle

# Features
DEFINES += "REDISMAP_SUPPORT_PROTOBUF"

# redis templates
include(RedisTemplates.pri)
