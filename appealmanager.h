#ifndef APPEALMANAGER_H
#define APPEALMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "database.h"
#include "sellernotifymanager.h"

// 申诉业务管理类（服务端，处理申诉相关业务逻辑）
class AppealManager : public QObject
{
    Q_OBJECT
public:
    explicit AppealManager(Database *db, SellerNotifyManager *sellerNotifyMgr, QObject *parent = nullptr);

    // 处理买家提交的申诉请求
    QJsonObject processBuyerAppeal(const QJsonObject &request);

    // 处理管理员查询待处理申诉的请求
    QJsonObject processGetPendingAppeals();

    // 处理管理员审核申诉的请求
    QJsonObject processHandleAppeal(const QJsonObject &request);

private:
    Database *m_database; // 数据库实例（复用原有数据库，不新建）

    // 生成唯一申诉ID
    QString generateAppealId();

    SellerNotifyManager *m_sellerNotifyMgr;// 卖家通知管理器实例
};

#endif
