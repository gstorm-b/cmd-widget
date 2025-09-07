QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    moveleditor.cpp \
    widget/command.cpp \
    widget/commandeditor.cpp \
    widget/commandeditorpanel.cpp \
    widget/commandmodel.cpp \
    widget/commandtreeview.cpp

HEADERS += \
    hyprgcommand.h \
    mainwindow.h \
    moveleditor.h \
    widget/command.h \
    widget/commandeditor.h \
    widget/commandeditorpanel.h \
    widget/commandmodel.h \
    widget/commandnode.h \
    widget/commandrowwidget.h \
    widget/commandtreeview.h \
    widget/rowdelegate.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
