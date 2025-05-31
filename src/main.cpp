// main.cpp
// Application entry point

#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Optional: Attempt to set a more modern style if available
    // QApplication::setStyle(QStyleFactory::create("Fusion"));
    // Or "Windows", "macOS" depending on the system and Qt version.
    // If not set, it uses the system's default style.

    MainWindow w;
    w.show();
    return a.exec();
} 