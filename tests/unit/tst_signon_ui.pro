include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TARGET = signon-ui-unittest

CONFIG += \
    build_all \
    debug \
    link_pkgconfig \
    qtestlib

QT += \
    core \
    dbus \
    gui \
    network \
    webkit

PKGCONFIG += \
    signon-plugins-common \
    libnotify

lessThan(QT_MAJOR_VERSION, 5) {
    PKGCONFIG += \
        accounts-qt \
        libsignon-qt
} else {
    QT += \
        webkitwidgets \
        widgets
    PKGCONFIG += \
        accounts-qt5 \
        libsignon-qt5
}

SOURCES += \
    fake-libnotify.cpp \
    fake-libsignon.cpp \
    fake-webcredentials-interface.cpp \
    test.cpp \
    $$TOP_SRC_DIR/src/animation-label.cpp \
    $$TOP_SRC_DIR/src/browser-request.cpp \
    $$TOP_SRC_DIR/src/cookie-jar-manager.cpp \
    $$TOP_SRC_DIR/src/debug.cpp \
    $$TOP_SRC_DIR/src/dialog-request.cpp \
    $$TOP_SRC_DIR/src/dialog.cpp \
    $$TOP_SRC_DIR/src/http-warning.cpp \
    $$TOP_SRC_DIR/src/i18n.cpp \
    $$TOP_SRC_DIR/src/indicator-service.cpp \
    $$TOP_SRC_DIR/src/network-access-manager.cpp \
    $$TOP_SRC_DIR/src/reauthenticator.cpp \
    $$TOP_SRC_DIR/src/request.cpp \
    $$TOP_SRC_DIR/src/webcredentials_adaptor.cpp
HEADERS += \
    fake-libnotify.h \
    fake-webcredentials-interface.h \
    test.h \
    $$TOP_SRC_DIR/src/animation-label.h \
    $$TOP_SRC_DIR/src/browser-request.h \
    $$TOP_SRC_DIR/src/debug.h \
    $$TOP_SRC_DIR/src/cookie-jar-manager.h \
    $$TOP_SRC_DIR/src/dialog-request.h \
    $$TOP_SRC_DIR/src/dialog.h \
    $$TOP_SRC_DIR/src/http-warning.h \
    $$TOP_SRC_DIR/src/indicator-service.h \
    $$TOP_SRC_DIR/src/network-access-manager.h \
    $$TOP_SRC_DIR/src/reauthenticator.h \
    $$TOP_SRC_DIR/src/request.h \
    $$TOP_SRC_DIR/src/webcredentials_adaptor.h

lessThan(QT_MAJOR_VERSION, 5) {
    SOURCES += $$TOP_SRC_DIR/src/embed-manager.cpp
    HEADERS += $$TOP_SRC_DIR/src/embed-manager.h
}

INCLUDEPATH += \
    . \
    $$TOP_SRC_DIR/src

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti

DEFINES += \
    DEBUG_ENABLED \
    UNIT_TESTS

RESOURCES += $$TOP_SRC_DIR/src/animationlabel.qrc

check.depends = $${TARGET}
check.commands = "xvfb-run -a dbus-test-runner -t ./signon-ui-unittest"
QMAKE_EXTRA_TARGETS += check
