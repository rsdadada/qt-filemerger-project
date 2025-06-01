QT       += core testlib # REMOVED gui
CONFIG   += console testcase # testcase auto-generates main() for tests
TARGET   = tst_customfilemodel

INCLUDEPATH += ../src

# Input
HEADERS += \
    ../src/customfilemodel.h \
    ../src/treeitem.h \
    ../src/filemergerlogic.h

SOURCES += \
    ../src/customfilemodel.cpp \
    ../src/treeitem.cpp \
    ../src/filemergerlogic.cpp \
    tst_customfilemodel.cpp
# Removed tst_filemergerlogic.cpp from here as it will be included by tst_customfilemodel.cpp

# If your customfilemodel.cpp or treeitem.cpp use tr() for strings that should be translated,
# you might need to add a TRANSLATIONS file here.
# For tests, it's often not critical unless testing localization itself.

# Default Qt Creator settings might put build artifacts in a build subfolder.
# The test executable will be named tst_customfilemodel (or tst_customfilemodel.exe on Windows) 