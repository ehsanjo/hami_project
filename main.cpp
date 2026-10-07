#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("PriceTracker"));
    QCoreApplication::setApplicationName(QStringLiteral("PriceTracker"));

    MainWindow window;
    window.resize(820, 380);
    window.show();

    return app.exec();
}