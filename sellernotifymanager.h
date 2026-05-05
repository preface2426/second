#ifndef SELLERNOTIFYMANAGER_H
#define SELLERNOTIFYMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "database.h"

// 卖家通知管理类（处理申诉结果推送、消息查询、已读标记）
class SellerNotifyManager : public QObject
{
    Q_OBJECT
public:
    explicit SellerNotifyManager(Database *db, QObject *parent = nullptr);

    // 1. 发送申诉结果通知给卖家（持久化消息+后续支持TCP推送）
    QJsonObject sendAppealResultToSeller(const QJsonObject &appealDetail, const QString &adminWarning);

    // 2. 处理卖家查询未读消息的请求
    QJsonObject processGetUnreadSellerMsg(const QJsonObject &request);

    // 3. 处理卖家标记消息为已读的请求
    QJsonObject processMarkSellerMsgAsRead(const QJsonObject &request);

private:
    Database *m_database; // 复用原有数据库实例，不新建
};

#endif // SELLERNOTIFYMANAGER_H
