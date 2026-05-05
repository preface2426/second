#include <QApplication>
#include <QFile>
#include <QDebug>
#include "adminwindow.h"

static void loadAppQss(QApplication &a)
{
    // 优先从资源加载（如果你有 qrc）
    QFile qss1(":/style.qss");
    if (qss1.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(qss1.readAll());
        qss1.close();
        qDebug() << "✅ QSS loaded from :/style.qss";
        return;
    }

    // 否则从工作目录加载（把 style.qss 放到 exe 同目录即可）
    QFile qss2("style.qss");
    if (qss2.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(qss2.readAll());
        qss2.close();
        qDebug() << "✅ QSS loaded from ./style.qss";
        return;
    }

    qDebug() << "❌ Failed to load QSS";
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    loadAppQss(a);

    AdminWindow w;
    w.show();
    return a.exec();
}
