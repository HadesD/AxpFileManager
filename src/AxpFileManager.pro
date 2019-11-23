#-------------------------------------------------
#
# Project created by QtCreator 2018-12-01T21:58:23
#
#-------------------------------------------------

QT       += core gui
win32:QT += winextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AxpFileManager
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += _CRT_SECURE_NO_WARNINGS
DEFINES -= UNICODE

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11
CONFIG -= app_bundle
CONFIG -= import_plugins

static: DEFINES += AXP_STATIC_LIB

INCLUDEPATH += $$PWD/../deps/LibAxpArchive/include

CONFIG(debug, debug|release) {
  CONFIGURATION = Debug
} else {
  CONFIGURATION = Release
}

LIBS += -L$$PWD/../deps/lib/$$CONFIGURATION
LIBS += -L$$PWD/../deps/LibAxpArchive/deps/lib/$$CONFIGURATION
LIBS += -lLibAxpArchive -lzzip -lzlib -lshlwapi

SOURCES += \
    AxpArchivePort.cpp \
    AxpDirListView.cpp \
    AxpFileListView.cpp \
    AxpItem.cpp \
    CustomLabel.cpp \
    StyledItemDelegate.cpp \
    Utils.cpp \
        main.cpp \
        MainWindow.cpp

HEADERS += \
    AxpArchivePort.hpp \
    AxpDirListView.hpp \
    AxpFileListView.hpp \
    AxpItem.hpp \
    CustomLabel.hpp \
        MainWindow.hpp \
    Log.hpp \
    StyledItemDelegate.hpp \
    Utils.hpp

FORMS += \
        MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    $$PWD/Resources/darkstyle/style.qrc

RC_FILE += \
    Resources.rc
