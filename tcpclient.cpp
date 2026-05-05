#include "tcpclient.h"
#include <QJsonDocument>
#include <QElapsedTimer>

TcpClient::TcpClient(QObject *parent) : QObject(parent), m_socket(new QTcpSocket(this)) { }

bool TcpClient::connectToServer(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
    return m_socket->waitForConnected(5000);
}

void TcpClient::close()
{
    m_socket->close();
}

QJsonObject TcpClient::sendRequest(const QJsonObject &req, int timeoutMs)
{
    QJsonDocument doc(req);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(data);
    m_socket->flush();

    QElapsedTimer t;
    t.start();

    QByteArray buf;
    while (t.elapsed() < timeoutMs) {
        if (!m_socket->waitForReadyRead(3000)) continue;
        buf += m_socket->readAll();

        int idx = buf.indexOf('\n');
        if (idx >= 0) {
            QByteArray line = buf.left(idx).trimmed();
            // buf.remove(0, idx+1); // 这里一般一问一答，留着也没事
            auto obj = QJsonDocument::fromJson(line).object();
            return obj;
        }
    }

    emit error("服务器未响应或响应超时");
    return {};
}
