#include "mainwindow.h"

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

#if defined(Q_OS_MACOS)
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
#endif

    QApplication::setApplicationName(QStringLiteral("Kurly"));
    QApplication::setOrganizationName(QStringLiteral("Bekind Labs"));
    // Matches kurly.desktop so Linux desktops (esp. Wayland) find the icon.
    QApplication::setDesktopFileName(QStringLiteral("kurly"));

    QIcon icon;
    for (int size : {16, 32, 48, 64, 128, 256, 512})
        icon.addFile(QStringLiteral(":/icons/png/kurly-%1.png").arg(size), QSize(size, size));
    QApplication::setWindowIcon(icon);

    MainWindow w;
    w.show();
    return app.exec();
}
