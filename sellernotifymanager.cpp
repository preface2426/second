#include "sellernotifymanager.h"
#include <QDateTime>

SellerNotifyManager::SellerNotifyManager(Database *db, QObject *parent)
    : QObject(parent), m_database(db)
{
    // 初始化卖家消息表（确保表存在）
    m_database->initSellerMsgTable();
}

// 1. 发送申诉结果通知给卖家
QJsonObject SellerNotifyManager::sendAppealResultToSeller(const QJsonObject &appealDetail, const QString &adminWarning)
{
    QJsonObject response;
    response["code"] = -1;

    // 验证参数完整性
    if (appealDetail.isEmpty() || adminWarning.isEmpty() || appealDetail["seller_id"].toInt() <= 0) {
        response["message"] = "通知参数不完整，无法发送给卖家";
        return response;
    }

    // 插入卖家消息（持久化存储，供卖家拉取）
    if (m_database->insertSellerAppealMsg(appealDetail, adminWarning)) {
        response["code"] = 0;
        response["message"] = "申诉结果已成功发送给卖家（待卖家查看）";
        // 扩展点：此处可添加TCP主动推送逻辑（若原有项目支持长连接），当前先实现「拉取模式」，兼容原有短连接架构
    } else {
        response["message"] = "申诉结果发送给卖家失败";
    }

    return response;
}

// 2. 处理卖家查询未读消息的请求
QJsonObject SellerNotifyManager::processGetUnreadSellerMsg(const QJsonObject &request)
{
    QJsonObject response;
    response["code"] = -1;

    // 提取卖家ID
    int sellerId = request["seller_id"].toInt();
    if (sellerId <= 0) {
        response["message"] = "卖家ID无效，无法查询未读消息";
        return response;
    }

    // 查询未读消息并返回
    QJsonArray unreadMsgArray = m_database->getUnreadSellerMsg(sellerId);
    response["code"] = 0;
    response["message"] = "查询成功";
    response["unread_msgs"] = unreadMsgArray;
    response["unread_count"] = unreadMsgArray.size();

    return response;
}

// 3. 处理卖家标记消息为已读的请求
QJsonObject SellerNotifyManager::processMarkSellerMsgAsRead(const QJsonObject &request)
{
    QJsonObject response;
    response["code"] = -1;

    // 提取消息ID
    QString msgId = request["msg_id"].toString();
    if (msgId.isEmpty()) {
        response["message"] = "消息ID无效，无法标记为已读";
        return response;
    }

    // 标记消息为已读
    if (m_database->markSellerMsgAsRead(msgId)) {
        response["code"] = 0;
        response["message"] = "消息已标记为已读";
    } else {
        response["message"] = "消息标记为已读失败";
    }

    return response;
}
