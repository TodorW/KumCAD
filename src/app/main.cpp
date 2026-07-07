#include "MainWindow.h"

#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("LinuxCAD"));

    MainWindow window;
    window.show();

    return app.exec();
}
