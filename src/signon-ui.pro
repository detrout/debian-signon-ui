include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)
include($${TOP_SRC_DIR}/common-installs-config.pri)

TEMPLATE = app
TARGET = signon-ui

I18N_DOMAIN = signon-ui

CONFIG += \
    link_pkgconfig \
    qt

QT += \
    core \
    dbus \
    gui \
    network \
    quick \
    webkit

PKGCONFIG += \
    signon-plugins-common \
    libnotify \
    libproxy-1.0

lessThan(QT_MAJOR_VERSION, 5) {
    PKGCONFIG += \
        accounts-qt \
        libsignon-qt \
        x11
} else {
    QT += \
        webkitwidgets \
        widgets
    PKGCONFIG += \
        accounts-qt5 \
        libsignon-qt5
    CONFIG(force-foreign-qwindow) {
        DEFINES += FORCE_FOREIGN_QWINDOW
    }
}

HEADERS = \
    animation-label.h \
    browser-request.h \
    cookie-jar-manager.h \
    debug.h \
    dialog-request.h \
    dialog.h \
    errors.h \
    http-warning.h \
    i18n.h \
    inactivity-timer.h \
    indicator-service.h \
    network-access-manager.h \
    reauthenticator.h \
    request.h \
    service.h \
    webcredentials_interface.h

SOURCES = \
    animation-label.cpp \
    browser-request.cpp \
    cookie-jar-manager.cpp \
    debug.cpp \
    dialog-request.cpp \
    dialog.cpp \
    http-warning.cpp \
    i18n.cpp \
    inactivity-timer.cpp \
    indicator-service.cpp \
    main.cpp \
    my-network-proxy-factory.cpp \
    network-access-manager.cpp \
    reauthenticator.cpp \
    request.cpp \
    service.cpp \
    webcredentials_interface.cpp

lessThan(QT_MAJOR_VERSION, 5) {
    HEADERS += embed-manager.h
    SOURCES += embed-manager.cpp
}

COMMANDLINE = ""

CONFIG(use-ubuntu-web-view) {
    DEFINES += USE_UBUNTU_WEB_VIEW
    HEADERS += \
        qquick-dialog.h \
        ubuntu-browser-request.h
    SOURCES += \
        qquick-dialog.cpp \
        ubuntu-browser-request.cpp
    OTHER_FILES += \
        qml/DefaultPage.qml \
        qml/KeyboardRectangle.qml \
        qml/MainWindow.qml \
        qml/StandardAnimation.qml \
        qml/WebView.qml
    RESOURCES += \
        qml/qml.qrc

    QMAKE_SUBSTITUTES += \
        signon-ui.desktop.in
    desktop.path = $${INSTALL_PREFIX}/share/applications
    desktop.files += signon-ui.desktop
    INSTALLS += desktop

    COMMANDLINE += " --desktop_file_hint=$${INSTALL_PREFIX}/share/applications/signon-ui.desktop"
}

DEFINES += \
    DEBUG_ENABLED \
    I18N_DOMAIN=\\\"$${I18N_DOMAIN}\\\"

RESOURCES += \
    animationlabel.qrc \
    http-warning.qrc

SIGNONUI_DBUS_ADAPTORS += \
    com.canonical.indicators.webcredentials.xml
SIGNONUI_DBUS_INCLUDES += \
    indicator-service.h

include(signonui_dbus_adaptor.pri)

po.target = ../po/signon-ui.pot
po.depends = $${SOURCES}
po.commands = xgettext -o $@ -d $${I18N_DOMAIN} --keyword=_ $^

QMAKE_EXTRA_TARGETS += \
    po

QMAKE_SUBSTITUTES += \
    com.canonical.indicators.webcredentials.service.in \
    com.nokia.singlesignonui.service.in

service.path = $${INSTALL_PREFIX}/share/dbus-1/services
service.files = \
    com.canonical.indicators.webcredentials.service \
    com.nokia.singlesignonui.service
INSTALLS += service

# Help file for HTTP authentication warning
!isEmpty(HTTP_WARNING_HELP) {
    DEFINES += HTTP_WARNING_HELP=\\\"$${HTTP_WARNING_HELP}\\\"
}
