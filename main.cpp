#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("PriceTracker"));
    QCoreApplication::setApplicationName(QStringLiteral("PriceTracker"));

    MainWindow window;
#ifdef Q_OS_ANDROID
    window.showMaximized();
#else
    window.resize(920, 700);
    window.show();
#endif

    return app.exec();
}