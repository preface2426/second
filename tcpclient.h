#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    bool connectToServer(const QString &host = "127.0.0.1", quint16 port = 12345);//121.199.24.155
    void close();
    QJsonObject sendRequest(const QJsonObject &req, int timeoutMs = 15000);  // 同步阻塞
signals:
    void error(const QString &msg);
private:
    QTcpSocket *m_socket;
};
