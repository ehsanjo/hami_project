#include <QApplication>

#include "mainwindow.h"
#ifdef Q_OS_ANDROID
#include "androidservice.h"
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    for (int i = 1; i < argc; ++i) {
        if (QByteArray(argv[i]) == "-service")
            return AndroidBridge::runService(argc, argv);
    }
#endif
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