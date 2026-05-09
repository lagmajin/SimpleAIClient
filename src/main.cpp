#include <QApplication>
#include <QCoreApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    qputenv("QT_NETWORK_SSL_BACKEND", "schannel");
    
    QApplication app(argc, argv);
    app.setApplicationName("SimpleAIClient");
    app.setOrganizationName("lagmajin");

    MainWindow window;
    window.show();

    return app.exec();
}
