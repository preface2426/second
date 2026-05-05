#include <QCoreApplication>
#include <QTcpServer>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 打开日志文件（会生成在项目编译目录下）
    QFile logFile("server_log.txt");
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream log(&logFile);
    log << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ";

    QTcpServer server;
    if (server.listen(QHostAddress::Any, 12345)) {
        log << "✅ 极简服务端启动成功！监听端口 12345\n";
    } else {
        log << "❌ 服务端启动失败：" << server.errorString() << "\n";
        return -1;
    }
    logFile.close();

    return a.exec();
}
