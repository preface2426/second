#include <QApplication>
#include <QFile>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qssFile(":/style.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(qssFile.readAll());
        qssFile.close();
        qDebug() << "✅ QSS loaded: :/style.qss";
    } else {
        qDebug() << "❌ Failed to load QSS: :/style.qss";
    }

    MainWindow w;
    w.show();
    return a.exec();
}
