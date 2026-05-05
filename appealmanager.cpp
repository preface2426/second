#include "appealmanager.h"
#include <QUuid>
#include <QDateTime>

AppealManager::AppealManager(Database *db, SellerNotifyManager *sellerNotifyMgr, QObject *parent)
    : QObject(parent), m_database(db), m_sellerNotifyMgr(sellerNotifyMgr)
{
    // 初始化申诉表（确保表存在）
    m_database->initAppealTable();
}

// 处理买家提交的申诉
QJsonObject AppealManager::processBuyerAppeal(const QJsonObject &request)
{
    QJsonObject response;
    response["code"] = -1; // 默认失败

    // 1. 提取请求参数
    QString sellerName = request["seller_name"].toString();
    QString bookTitle = request["book_title"].toString();
    QString bookId = request["book_id"].toString();
    QString appealReason = request["appeal_reason"].toString();
    QString compensationType = request["compensation_type"].toString();
    int buyerId = request["buyer_id"].toInt();

    // 2. 验证参数是否完整
    if (sellerName.isEmpty() || bookTitle.isEmpty() || bookId.isEmpty() ||
        appealReason.isEmpty() || compensationType.isEmpty() || buyerId <= 0) {
        response["message"] = "参数不完整，请补充所有必填信息";
        return response;
    }

    // 3. 验证申诉信息是否属实
    if (!m_database->verifyAppealInfo(sellerName, bookTitle, bookId)) {
        response["message"] = "申诉信息不实，卖家或书籍信息不存在，驳回申诉";
        return response;
    }

    // 4. 构造申诉数据
    QJsonObject appealData;
    appealData["appeal_id"] = generateAppealId();
    appealData["buyer_id"] = buyerId;
    appealData["seller_name"] = sellerName;
    appealData["seller_id"] = m_database->getSellerIdByUserName(sellerName);
    appealData["book_id"] = bookId;
    appealData["book_title"] = bookTitle;
    appealData["appeal_reason"] = appealReason;
    appealData["compensation_type"] = compensationType;
    appealData["appeal_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // 5. 提交申诉到数据库
    if (m_database->submitAppeal(appealData)) {
        response["code"] = 0;
        response["message"] = "申诉信息验证通过，已提交管理员审核";
        response["appeal_id"] = appealData["appeal_id"];
    } else {
        response["message"] = "申诉提交失败，请稍后重试";
    }

    return response;
}

// 处理管理员查询待处理申诉
QJsonObject AppealManager::processGetPendingAppeals()
{
    QJsonObject response;
    response["code"] = 0;
    response["data"] = m_database->getPendingAppeals();
    return response;
}

// 处理管理员审核申诉
QJsonObject AppealManager::processHandleAppeal(const QJsonObject &request)
{
    QJsonObject response;
    response["code"] = -1;

    // 提取参数
    QString appealId = request["appeal_id"].toString();
    int status = request["status"].toInt();
    QString handleOpinion = request["handle_opinion"].toString();

    if (appealId.isEmpty() || handleOpinion.isEmpty()) {
        response["message"] = "参数不完整，无法处理申诉";
        return response;
    }

    // 处理申诉（更新数据库状态）
    if (m_database->handleAppeal(appealId, (AppealStatus)status, handleOpinion)) {
        response["code"] = 0;
        response["message"] = "申诉处理完成";
        // 返回申诉详情（用于通知卖家）
        response["appeal_detail"] = m_database->getAppealDetail(appealId);
        if (status == APPEAL_APPROVED && m_sellerNotifyMgr != nullptr) {
            // 提取申诉详情
            QJsonObject appealDetail = m_database->getAppealDetail(appealId);
            // 构造管理员警告（整合处理意见，明确警告内容）
            QString adminWarning = QString("【管理员警告】\n你的商品涉及买家申诉，审核已通过。\n管理员意见：%1\n请你尽快处理买家的赔偿要求，否则将影响你的平台使用权限。")
                                       .arg(handleOpinion);
            // 发送通知给卖家
            QJsonObject notifyResponse = m_sellerNotifyMgr->sendAppealResultToSeller(appealDetail, adminWarning);
            // 将通知结果附加到响应中（供管理员查看）
            response["seller_notify_result"] = notifyResponse["message"];
        }
    } else {
        response["message"] = "申诉处理失败，请稍后重试";
    }

    return response;
}

// 生成唯一申诉ID
QString AppealManager::generateAppealId()
{
    return "AP_" + QUuid::createUuid().toString().replace("-", "").mid(0, 20);
}
