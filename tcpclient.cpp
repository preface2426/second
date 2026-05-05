#include "tcpclient.h"
#include <QElapsedTimer>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
}

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
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit error("未连接服务器");
        return {};
    }

    QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n";
    if (m_socket->write(payload) < 0) {
        emit error("写入失败");
        return {};
    }
    if (!m_socket->waitForBytesWritten(3000)) {
        emit error("写入超时");
        return {};
    }

    QByteArray buf;
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        if (!m_socket->waitForReadyRead(200)) {
            continue;
        }
        buf += m_socket->readAll();

        int idx = buf.indexOf('\n');
        if (idx >= 0) {
            QByteArray line = buf.left(idx).trimmed();
            QJsonDocument doc = QJsonDocument::fromJson(line);
            if (!doc.isObject()) {
                emit error("响应格式错误");
                return {};
            }
            return doc.object();
        }
    }

    emit error("服务器未响应或响应超时");
    return {};
}
