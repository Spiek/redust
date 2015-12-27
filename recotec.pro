# Features
DEFINES += "REDISMAP_SUPPORT_PROTOBUF"

# Lib settings
TARGET   = recotec
TEMPLATE = lib
CONFIG   += console
CONFIG   -= app_bundle

# Include recotec
include(recotec.pri)

# Binary installer
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# Header installer
header.files = $$HEADERS
header.path = $$[QT_INSTALL_HEADERS]/recotec
INSTALLS += header
