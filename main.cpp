#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("PriceTracker"));
    QCoreApplication::setApplicationName(QStringLiteral("PriceTracker"));

    MainWindow window;
    window.resize(920, 700);
    window.show();

    return app.exec();
}