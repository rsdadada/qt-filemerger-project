QT       += core gui widgets
TARGET = FileMergerApp
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/filemergerlogic.cpp \
    src/customfilemodel.cpp \
    src/treeitem.cpp \
    src/helpdialog.cpp

HEADERS  += \
    src/mainwindow.h \
    src/filemergerlogic.h \
    src/customfilemodel.h \
    src/treeitem.h \
    src/helpdialog.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target 
