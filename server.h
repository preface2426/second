#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QHash>

#include "database.h"
#include "sellernotifymanager.h"
#include "appealmanager.h"

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    bool start(quint16 port);

protected:
    void incomingConnection(qintptr handle) override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    Database m_db;
    QHash<QTcpSocket*, QByteArray> m_buffers;

    // ✅ 新增：两个业务管理器
    SellerNotifyManager *m_sellerNotifyMgr = nullptr;
    AppealManager *m_appealMgr = nullptr;

    void processRequest(QTcpSocket *socket, const QJsonObject &req);
    void sendJson(QTcpSocket *socket, const QJsonObject &obj);
};
