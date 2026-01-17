QT       += core gui widgets sql

greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

CONFIG += c++17

TARGET = HolyBooksExplorer
TEMPLATE = app

# Application version
VERSION = 1.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Sources
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    databasemanager.cpp \
    settingsdialog.cpp \
    searchhighlighter.cpp \
    versewidget.cpp \
    wordanalysiswidget.cpp

HEADERS += \
    mainwindow.h \
    databasemanager.h \
    settingsdialog.h \
    searchhighlighter.h \
    versewidget.h \
    wordanalysiswidget.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

# Resources
RESOURCES += resources.qrc

# Translations
TRANSLATIONS += \
    translations/holybooksexplorer_tr.ts \
    translations/holybooksexplorer_en.ts

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Windows specific
win32 {
    RC_ICONS = icons/app.ico
}

# macOS specific
macx {
    ICON = icons/app.icns
}
