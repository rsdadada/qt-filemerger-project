QT       += core gui widgets
TARGET = FileMergerApp
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    filemergerlogic.cpp \
    customfilemodel.cpp \
    treeitem.cpp

HEADERS  += \
    mainwindow.h \
    filemergerlogic.h \
    customfilemodel.h \
    treeitem.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target 
