#include <QApplication>
#include <QCoreApplication>
#include "mainwindow.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QTableWidget>
#include <QScrollBar>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SimpleAIClient");
    app.setOrganizationName("lagmajin");

    MainWindow window;
    window.show();

    return app.exec();
}
