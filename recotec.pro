# Features
DEFINES += "REDISMAP_SUPPORT_PROTOBUF"

# Lib settings
TARGET   = librecotec
TEMPLATE = lib
CONFIG   += console
CONFIG   -= app_bundle

# Include recotec
include(recotec.pri)
