#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Kurly"));
    QApplication::setOrganizationName(QStringLiteral("Bekind Labs"));

    MainWindow w;
    w.show();
    return app.exec();
}
