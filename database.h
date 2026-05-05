#pragma once
#include <QSqlDatabase>
#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

//*新增内容
enum AppealStatus {
    APPEAL_PENDING = 0,    // 待管理员审核
    APPEAL_APPROVED = 1,   // 管理员审核通过
    APPEAL_REJECTED = 2,   // 管理员审核驳回
    APPEAL_FINISHED = 3    // 申诉处理完成（已通知卖家）
};

enum SellerMsgStatus {
    MSG_UNREAD = 0,    // 未读
    MSG_READ = 1       // 已读
};

class Database
{
public:
    bool init();
    bool verifyAppealInfoBySellerId(int sellerId, const QString &bookTitle, const QString &bookId);

    // users
    bool registerUser(const QString &name, const QString &pwd, const QString &alipay);
    int login(const QString &name, const QString &pwd);

    // books
    bool addBook(const QString &title, const QString &cat, double price, int seller, const QString &img);
    QJsonArray listBooks();
    QJsonArray searchBooks(const QString &kw);
    QJsonArray booksBySeller(int sellerId);
    QJsonArray booksByBuyer(int buyerId);


    bool markBookSold(int bookId, int buyerId);
    // books
    bool deleteBook(int bookId, QString *sellerAlipay = nullptr);

    // chat
    int addMessage(int bookId, int fromUser, int toUser, const QString &text);
    QJsonArray fetchMessages(int userId, int peerId, int bookId, int afterId);
    void markMessagesRead(int userId, int peerId, int bookId);
    QJsonArray listChatSessions(int userId);

    //*新增内容

    QSqlDatabase& database() { return m_db; }

    // 1. 初始化申诉表（若不存在则创建）
    bool initAppealTable();

    // 2. 买家提交申诉（插入申诉数据）
    bool submitAppeal(const QJsonObject &appealData);

    // 3. 验证买家提交的申诉信息是否属实（验证卖家、书籍信息）
    bool verifyAppealInfo(const QString &sellerName, const QString &bookTitle, const QString &bookId);

    // 4. 管理员查询所有待处理申诉
    QJsonArray getPendingAppeals();

    // 5. 管理员处理申诉（更新申诉状态、添加处理意见）
    bool handleAppeal(const QString &appealId, AppealStatus status, const QString &handleOpinion);

    // 6. 查询指定申诉详情（用于转发给卖家）
    QJsonObject getAppealDetail(const QString &appealId);

    // 7. 获取卖家ID（通过卖家用户名，用于后续通知卖家）
    int getSellerIdByUserName(const QString &sellerName);

    // 8. 初始化卖家消息表（存储申诉结果通知）
    bool initSellerMsgTable();

    // 9. 插入卖家申诉通知消息（持久化存储）
    bool insertSellerAppealMsg(const QJsonObject &appealDetail, const QString &adminWarning);

    // 10. 查询指定卖家的未读消息（供卖家客户端拉取）
    QJsonArray getUnreadSellerMsg(int sellerId);

    // 11. 标记消息为已读（卖家查看后更新状态）
    bool markSellerMsgAsRead(const QString &msgId);

private:
    QSqlDatabase m_db;
};
