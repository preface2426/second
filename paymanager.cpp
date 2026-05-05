#include "paymanager.h"
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QFile>

PayManager::PayManager(QObject *parent) : QObject(parent) {}

void PayManager::createPayment(QString orderId, double price, QString title) {
    QString appPath = QCoreApplication::applicationDirPath();
    QString scriptPath = appPath + "/alipay_sign.py";
    if (!QFile::exists(scriptPath)) {
        emit errorOccurred("找不到脚本文件，请检查路径: " + scriptPath);
        return;
    }
    QProcess *process = new QProcess(this);
    // 强制使用 UTF-8 编码
    process->setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    args << scriptPath << orderId << QString::number(price, 'f', 2) << title;
    qDebug() << "执行命令: python " << args.join(" ");
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
                QString output = QString::fromUtf8(process->readAllStandardOutput().trimmed());
                if (output.isEmpty()) {
                    output = QString::fromUtf8(process->readAllStandardError().trimmed());
                }
                qDebug() << "Python 最终输出内容:" << output;
                if (output.startsWith("https://")) {
                    QDesktopServices::openUrl(QUrl(output));
                    emit urlReady(output);
                } else {
                    emit errorOccurred("支付初始化失败: " + output);
                }
                process->deleteLater();
            });
    QString pythonPath = "C:/Users/Alex/AppData/Local/Programs/Python/Python314/python.exe";
    if (!QFile::exists(pythonPath)) {
        emit errorOccurred("找不到 Python 解释器，请检查路径: " + pythonPath);
        return;
    }
    qDebug() << "尝试使用绝对路径启动: " << pythonPath;
    process->start(pythonPath, args);
}


