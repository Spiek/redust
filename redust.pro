# Features
DEFINES += "REDISMAP_SUPPORT_PROTOBUF"

# Lib settings
PROJECTNAME = redust
TEMPLATE = lib
CONFIG   += console debug_and_release
CONFIG   -= app_bundle

# Include redust
include(redust.pri)

# Binary installer
target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target
!build_pass:message("library binary install folder: "$$target.path)

# Header installer
header.files = $$HEADERS
header.path = $$[QT_INSTALL_HEADERS]/redust
INSTALLS += header
!build_pass:message("library headers install folder: "$$header.path)

# in debug mode append a d to target binary name
CONFIG(debug, debug|release) {
    TARGET = $${PROJECTNAME}"d"
}
CONFIG(release, debug|release) {
    TARGET = $${PROJECTNAME}
}
