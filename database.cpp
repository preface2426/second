#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonObject>

bool Database::init()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("secondhand.db");
    if (!m_db.open()) return false;

    QSqlQuery q(m_db);

    q.exec("CREATE TABLE IF NOT EXISTS users("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "username TEXT UNIQUE,"
           "password TEXT,"
           "alipay TEXT)");

    q.exec("CREATE TABLE IF NOT EXISTS books("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "title TEXT, category TEXT, price REAL, "
           "seller_id INTEGER, is_sold INTEGER DEFAULT 0, "
           "buyer_id INTEGER, image_path TEXT, "
           "created_time DATETIME DEFAULT CURRENT_TIMESTAMP)");

    // chat messages
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS messages(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            book_id INTEGER NOT NULL,
            from_user INTEGER NOT NULL,
            to_user INTEGER NOT NULL,
            text TEXT NOT NULL,
            is_read INTEGER DEFAULT 0,
            created_time TEXT DEFAULT (datetime('now','localtime'))
        )
    )");

    // 如果旧表没有 is_read，补列（失败也没关系）
    q.exec("ALTER TABLE messages ADD COLUMN is_read INTEGER DEFAULT 0");

    return true;
}

/* ========== users ========== */
bool Database::registerUser(const QString &name, const QString &pwd, const QString &alipay)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO users(username,password,alipay) VALUES(?,?,?)");
    q.addBindValue(name);
    q.addBindValue(pwd);
    q.addBindValue(alipay);
    return q.exec();
}

int Database::login(const QString &name, const QString &pwd)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT id FROM users WHERE username=? AND password=?");
    q.addBindValue(name);
    q.addBindValue(pwd);
    q.exec();
    if (q.next()) return q.value(0).toInt();
    return -1;
}

/* ========== books ========== */
bool Database::addBook(const QString &title, const QString &cat, double price, int seller, const QString &img)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO books(title,category,price,seller_id,image_path) VALUES(?,?,?,?,?)");
    q.addBindValue(title);
    q.addBindValue(cat);
    q.addBindValue(price);
    q.addBindValue(seller);
    q.addBindValue(img);
    return q.exec();
}

static QJsonObject bookRowToJson(QSqlQuery &q)
{
    QJsonObject o;
    o["id"] = q.value("id").toInt();
    o["title"] = q.value("title").toString();
    o["price"] = q.value("price").toDouble();
    o["is_sold"] = q.value("is_sold").toInt();
    o["seller_id"] = q.value("seller_id").toInt();
    o["buyer_id"] = q.value("buyer_id").isNull() ? 0 : q.value("buyer_id").toInt();
    o["image_path"] = q.value("image_path").toString();
    o["category"] = q.value("category").toString();
    o["created_time"] = q.value("created_time").toString();
    o["seller_name"] = q.value("seller_name").toString();

    return o;
}

QJsonArray Database::listBooks()
{
    QJsonArray arr;
    QSqlQuery q(m_db);
    q.exec("SELECT b.*, u.username AS seller_name "
           "FROM books b LEFT JOIN users u ON u.id=b.seller_id "
           "ORDER BY b.created_time DESC");

    while (q.next()) arr.append(bookRowToJson(q));
    return arr;
}

QJsonArray Database::searchBooks(const QString &kw)
{
    QJsonArray arr;
    QSqlQuery q(m_db);
    q.prepare("SELECT b.*, u.username AS seller_name "
              "FROM books b LEFT JOIN users u ON u.id=b.seller_id "
              "WHERE b.title LIKE ? ORDER BY b.created_time DESC");
    q.addBindValue("%" + kw + "%");
    q.exec("SELECT b.*, u.username AS seller_name "
           "FROM books b LEFT JOIN users u ON u.id=b.seller_id "
           "ORDER BY b.created_time DESC");

    while (q.next()) arr.append(bookRowToJson(q));
    return arr;
}

QJsonArray Database::booksBySeller(int sellerId)
{
    QJsonArray arr;
    QSqlQuery q(m_db);
    q.prepare("SELECT b.*, u.username AS seller_name "
              "FROM books b LEFT JOIN users u ON u.id=b.seller_id "
              "WHERE b.seller_id=? ORDER BY b.created_time DESC");
    q.addBindValue(sellerId);
    q.exec("SELECT b.*, u.username AS seller_name "
           "FROM books b LEFT JOIN users u ON u.id=b.seller_id "
           "ORDER BY b.created_time DESC");

    while (q.next()) arr.append(bookRowToJson(q));
    return arr;
}
QJsonArray Database::booksByBuyer(int buyerId)
{
    QJsonArray arr;
    QSqlQuery q(m_db);
    q.prepare("SELECT b.*, u.username AS seller_name "
              "FROM books b LEFT JOIN users u ON u.id=b.seller_id "
              "WHERE b.is_sold=1 AND b.buyer_id=? "
              "ORDER BY b.created_time DESC");
    q.addBindValue(buyerId);
    q.exec();
    while (q.next()) arr.append(bookRowToJson(q));
    return arr;
}

bool Database::markBookSold(int bookId, int buyerId)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE books SET is_sold=1, buyer_id=? WHERE id=? AND is_sold=0");
    q.addBindValue(buyerId);
    q.addBindValue(bookId);
    return q.exec() && q.numRowsAffected() > 0;
}

/* ========== chat ========== */
int Database::addMessage(int bookId, int fromUser, int toUser, const QString &text)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO messages(book_id, from_user, to_user, text, is_read) VALUES(?,?,?,?,0)");
    q.addBindValue(bookId);
    q.addBindValue(fromUser);
    q.addBindValue(toUser);
    q.addBindValue(text);
    if (!q.exec()) return -1;
    return q.lastInsertId().toInt();
}

QJsonArray Database::fetchMessages(int userId, int peerId, int bookId, int afterId)
{
    QJsonArray arr;
    QSqlQuery q(m_db);
    q.prepare(R"(
      SELECT id, book_id, from_user, to_user, text, created_time
      FROM messages
      WHERE book_id=?
        AND id>?
        AND (
          (from_user=? AND to_user=?)
          OR
          (from_user=? AND to_user=?)
        )
      ORDER BY id ASC
    )");
    q.addBindValue(bookId);
    q.addBindValue(afterId);
    q.addBindValue(userId);
    q.addBindValue(peerId);
    q.addBindValue(peerId);
    q.addBindValue(userId);

    if (!q.exec()) return arr;

    while (q.next()) {
        QJsonObject m;
        m["id"] = q.value(0).toInt();
        m["book_id"] = q.value(1).toInt();
        m["from"] = q.value(2).toInt();
        m["to"] = q.value(3).toInt();
        m["text"] = q.value(4).toString();
        m["time"] = q.value(5).toString();
        arr.append(m);
    }
    return arr;
}

void Database::markMessagesRead(int userId, int peerId, int bookId)
{
    QSqlQuery q(m_db);
    q.prepare(R"(
      UPDATE messages
      SET is_read=1
      WHERE book_id=?
        AND to_user=?
        AND from_user=?
        AND is_read=0
    )");
    q.addBindValue(bookId);
    q.addBindValue(userId);
    q.addBindValue(peerId);
    q.exec();
}
bool Database::deleteBook(int bookId, QString *sellerAlipay)
{
    if (!m_db.isOpen()) return false;

    QSqlQuery q(m_db);

    // 1) 先查卖家支付宝（给管理员提示用）
    q.prepare(R"(
        SELECT u.alipay
        FROM books b
        LEFT JOIN users u ON u.id = b.seller_id
        WHERE b.id = ?
    )");
    q.addBindValue(bookId);
    if (!q.exec()) {
        qDebug() << "deleteBook select failed:" << q.lastError().text();
        return false;
    }
    if (!q.next()) {
        // 没找到该书
        return false;
    }
    if (sellerAlipay) *sellerAlipay = q.value(0).toString();

    // 2) 事务：先删消息，再删书
    if (!m_db.transaction()) {
        qDebug() << "deleteBook transaction failed:" << m_db.lastError().text();
        // 没事务也继续尝试
    }

    // 删除该书的聊天记录（避免遗留垃圾数据）
    QSqlQuery q1(m_db);
    q1.prepare("DELETE FROM messages WHERE book_id = ?");
    q1.addBindValue(bookId);
    if (!q1.exec()) {
        qDebug() << "deleteBook delete messages failed:" << q1.lastError().text();
        m_db.rollback();
        return false;
    }

    // 删除书籍
    QSqlQuery q2(m_db);
    q2.prepare("DELETE FROM books WHERE id = ?");
    q2.addBindValue(bookId);
    if (!q2.exec()) {
        qDebug() << "deleteBook delete book failed:" << q2.lastError().text();
        m_db.rollback();
        return false;
    }

    bool ok = (q2.numRowsAffected() > 0);
    if (ok) m_db.commit();
    else m_db.rollback();

    return ok;
}
QJsonArray Database::listChatSessions(int userId)
{
    QJsonArray arr;
    QSqlQuery q(m_db);

    // 会话：book_id + peer_id（对方）
    q.prepare(R"(
      SELECT
        book_id,
        CASE WHEN from_user=? THEN to_user ELSE from_user END AS peer_id,
        MAX(id) AS last_id,
        MAX(created_time) AS last_time,
        SUM(CASE WHEN to_user=? AND is_read=0 THEN 1 ELSE 0 END) AS unread_count
      FROM messages
      WHERE from_user=? OR to_user=?
      GROUP BY book_id, peer_id
      ORDER BY last_id DESC
    )");
    q.addBindValue(userId);
    q.addBindValue(userId);
    q.addBindValue(userId);
    q.addBindValue(userId);

    if (!q.exec()) return arr;

    while (q.next()) {
        int bookId = q.value("book_id").toInt();
        int peerId = q.value("peer_id").toInt();
        int lastId = q.value("last_id").toInt();
        QString lastTime = q.value("last_time").toString();
        int unread = q.value("unread_count").toInt();

        // last text / last from
        QString lastText;
        int lastFrom = 0;
        {
            QSqlQuery q2(m_db);
            q2.prepare("SELECT text, from_user FROM messages WHERE id=?");
            q2.addBindValue(lastId);
            if (q2.exec() && q2.next()) {
                lastText = q2.value(0).toString();
                lastFrom = q2.value(1).toInt();
            }
        }

        // book title
        QString bookTitle;
        {
            QSqlQuery qb(m_db);
            qb.prepare("SELECT title FROM books WHERE id=?");
            qb.addBindValue(bookId);
            if (qb.exec() && qb.next()) bookTitle = qb.value(0).toString();
        }

        QJsonObject o;
        o["book_id"] = bookId;
        o["peer_id"] = peerId;
        o["last_id"] = lastId;
        o["last_time"] = lastTime;
        o["unread"] = unread;
        o["last_text"] = lastText;
        o["last_from"] = lastFrom;
        o["book_title"] = bookTitle;
        arr.append(o);
    }

    return arr;
}

// 1. 初始化申诉表
bool Database::initAppealTable()
{
    QSqlQuery query;
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS appeals (
            appeal_id TEXT PRIMARY KEY,
            buyer_id INTEGER NOT NULL,
            seller_name TEXT NOT NULL,
            seller_id INTEGER NOT NULL,
            book_id TEXT NOT NULL,
            book_title TEXT NOT NULL,
            appeal_reason TEXT NOT NULL,
            compensation_type TEXT NOT NULL, -- "退货退款" / "更换同规格合格商品"
            appeal_time DATETIME NOT NULL,
            status INTEGER DEFAULT 0, -- 对应AppealStatus枚举
            handle_opinion TEXT DEFAULT "",
            handle_time DATETIME DEFAULT "",
            FOREIGN KEY (buyer_id) REFERENCES users(id),
            FOREIGN KEY (seller_id) REFERENCES users(id),
            FOREIGN KEY (book_id) REFERENCES books(id)
        )
    )";

    if (!query.exec(createSql)) {
        qDebug() << "创建申诉表失败：" << query.lastError().text();
        return false;
    }
    return true;
}

// 2. 买家提交申诉
bool Database::submitAppeal(const QJsonObject &appealData)
{
    if (!this->database().transaction()) {
        qDebug() << "开启申诉事务失败";
        return false;
    }

    QSqlQuery query;
    QString insertSql = R"(
        INSERT INTO appeals (
            appeal_id, buyer_id, seller_name, seller_id, book_id,
            book_title, appeal_reason, compensation_type, appeal_time, status
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    query.prepare(insertSql);
    query.bindValue(0, appealData["appeal_id"].toString());
    query.bindValue(1, appealData["buyer_id"].toInt());
    query.bindValue(2, appealData["seller_name"].toString());
    query.bindValue(3, appealData["seller_id"].toInt());
    query.bindValue(4, appealData["book_id"].toString());
    query.bindValue(5, appealData["book_title"].toString());
    query.bindValue(6, appealData["appeal_reason"].toString());
    query.bindValue(7, appealData["compensation_type"].toString());
    query.bindValue(8, appealData["appeal_time"].toString());
    query.bindValue(9, APPEAL_PENDING);

    if (!query.exec()) {
        qDebug() << "提交申诉失败：" << query.lastError().text();
        this->database().rollback();
        return false;
    }

    return this->database().commit();
}

// 3. 验证买家提交的申诉信息是否属实
bool Database::verifyAppealInfo(const QString &sellerName, const QString &bookTitle, const QString &bookId)
{
    QSqlQuery query;
    // 1. 验证书籍是否存在
    QString bookSql = "SELECT * FROM books WHERE id = ? AND title = ?";
    query.prepare(bookSql);
    query.bindValue(0, bookId);
    query.bindValue(1, bookTitle);
    if (!query.exec() || !query.next()) {
        qDebug() << "书籍信息验证失败";
        return false;
    }

    // 2. 验证卖家是否存在且与书籍关联
    QString sellerSql = "SELECT u.id FROM users u JOIN books b ON u.id = b.seller_id WHERE u.username = ? AND b.id = ?";
    query.prepare(sellerSql);
    query.bindValue(0, sellerName);
    query.bindValue(1, bookId);
    if (!query.exec() || !query.next()) {
        qDebug() << "卖家信息验证失败";
        return false;
    }

    return true;
}

// 4. 管理员查询所有待处理申诉
QJsonArray Database::getPendingAppeals()
{
    QJsonArray appealArray;
    QSqlQuery query;
    QString sql = "SELECT * FROM appeals WHERE status = ? ORDER BY appeal_time DESC";
    query.prepare(sql);
    query.bindValue(0, APPEAL_PENDING);

    if (!query.exec()) {
        qDebug() << "查询待处理申诉失败：" << query.lastError().text();
        return appealArray;
    }

    while (query.next()) {
        QJsonObject appealObj;
        appealObj["appeal_id"] = query.value("appeal_id").toString();
        appealObj["buyer_id"] = query.value("buyer_id").toInt();
        appealObj["seller_name"] = query.value("seller_name").toString();
        appealObj["book_id"] = query.value("book_id").toString();
        appealObj["book_title"] = query.value("book_title").toString();
        appealObj["appeal_reason"] = query.value("appeal_reason").toString();
        appealObj["compensation_type"] = query.value("compensation_type").toString();
        appealObj["appeal_time"] = query.value("appeal_time").toString();
        appealObj["status"] = query.value("status").toInt();
        appealArray.append(appealObj);
    }

    return appealArray;
}

// 5. 管理员处理申诉
bool Database::handleAppeal(const QString &appealId, AppealStatus status, const QString &handleOpinion)
{
    QSqlQuery query;
    QString updateSql = "UPDATE appeals SET status = ?, handle_opinion = ?, handle_time = ? WHERE appeal_id = ?";
    query.prepare(updateSql);
    query.bindValue(0, status);
    query.bindValue(1, handleOpinion);
    query.bindValue(2, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.bindValue(3, appealId);

    if (!query.exec()) {
        qDebug() << "处理申诉失败：" << query.lastError().text();
        return false;
    }

    return true;
}

// 6. 查询指定申诉详情
QJsonObject Database::getAppealDetail(const QString &appealId)
{
    QJsonObject appealObj;
    QSqlQuery query;
    QString sql = "SELECT * FROM appeals WHERE appeal_id = ?";
    query.prepare(sql);
    query.bindValue(0, appealId);

    if (!query.exec() || !query.next()) {
        qDebug() << "查询申诉详情失败：" << query.lastError().text();
        return appealObj;
    }

    appealObj["appeal_id"] = query.value("appeal_id").toString();
    appealObj["buyer_id"] = query.value("buyer_id").toInt();
    appealObj["seller_name"] = query.value("seller_name").toString();
    appealObj["seller_id"] = query.value("seller_id").toInt();
    appealObj["book_id"] = query.value("book_id").toString();
    appealObj["book_title"] = query.value("book_title").toString();
    appealObj["appeal_reason"] = query.value("appeal_reason").toString();
    appealObj["compensation_type"] = query.value("compensation_type").toString();
    appealObj["appeal_time"] = query.value("appeal_time").toString();
    appealObj["status"] = query.value("status").toInt();
    appealObj["handle_opinion"] = query.value("handle_opinion").toString();
    appealObj["handle_time"] = query.value("handle_time").toString();

    return appealObj;
}

// 7. 获取卖家ID（通过用户名）
int Database::getSellerIdByUserName(const QString &sellerName)
{
    QSqlQuery query;
    QString sql = "SELECT id FROM users WHERE username = ?";
    query.prepare(sql);
    query.bindValue(0, sellerName);

    if (!query.exec() || !query.next()) {
        qDebug() << "获取卖家ID失败：" << query.lastError().text();
        return -1;
    }

    return query.value("id").toInt();
}

// 8. 初始化卖家消息表
bool Database::initSellerMsgTable()
{
    QSqlQuery query;
    QString createSql = R"(
        CREATE TABLE IF NOT EXISTS seller_messages (
            msg_id TEXT PRIMARY KEY,
            seller_id INTEGER NOT NULL,
            appeal_id TEXT NOT NULL,
            appeal_content TEXT NOT NULL,
            admin_warning TEXT NOT NULL,
            send_time DATETIME NOT NULL,
            status INTEGER DEFAULT 0, -- 对应SellerMsgStatus枚举
            FOREIGN KEY (seller_id) REFERENCES users(id),
            FOREIGN KEY (appeal_id) REFERENCES appeals(appeal_id)
        )
    )";

    if (!query.exec(createSql)) {
        qDebug() << "创建卖家消息表失败：" << query.lastError().text();
        return false;
    }
    return true;
}

// 9. 插入卖家申诉通知消息（持久化）
bool Database::insertSellerAppealMsg(const QJsonObject &appealDetail, const QString &adminWarning)
{
    if (!this->database().transaction()) {
        qDebug() << "开启卖家消息事务失败";
        return false;
    }

    QSqlQuery query;
    QString insertSql = R"(
        INSERT INTO seller_messages (
            msg_id, seller_id, appeal_id, appeal_content, admin_warning, send_time, status
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
    )";

    // 生成唯一消息ID
    QString msgId = "MSG_" + QUuid::createUuid().toString().replace("-", "").mid(0, 20);
    // 构造申诉内容（整合买家申诉理由、赔偿要求）
    QString appealContent = QString("买家申诉详情：\n书籍ID：%1\n书籍标题：%2\n赔偿要求：%3\n申诉理由：%4")
                                .arg(appealDetail["book_id"].toString())
                                .arg(appealDetail["book_title"].toString())
                                .arg(appealDetail["compensation_type"].toString())
                                .arg(appealDetail["appeal_reason"].toString());

    query.prepare(insertSql);
    query.bindValue(0, msgId);
    query.bindValue(1, appealDetail["seller_id"].toInt());
    query.bindValue(2, appealDetail["appeal_id"].toString());
    query.bindValue(3, appealContent);
    query.bindValue(4, adminWarning);
    query.bindValue(5, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.bindValue(6, MSG_UNREAD);

    if (!query.exec()) {
        qDebug() << "插入卖家申诉消息失败：" << query.lastError().text();
        this->database().rollback();
        return false;
    }

    return this->database().commit();
}

// 10. 查询指定卖家的未读消息
QJsonArray Database::getUnreadSellerMsg(int sellerId)
{
    QJsonArray msgArray;
    QSqlQuery query;
    QString sql = "SELECT * FROM seller_messages WHERE seller_id = ? AND status = ? ORDER BY send_time DESC";
    query.prepare(sql);
    query.bindValue(0, sellerId);
    query.bindValue(1, MSG_UNREAD);

    if (!query.exec()) {
        qDebug() << "查询卖家未读消息失败：" << query.lastError().text();
        return msgArray;
    }

    while (query.next()) {
        QJsonObject msgObj;
        msgObj["msg_id"] = query.value("msg_id").toString();
        msgObj["seller_id"] = query.value("seller_id").toInt();
        msgObj["appeal_id"] = query.value("appeal_id").toString();
        msgObj["appeal_content"] = query.value("appeal_content").toString();
        msgObj["admin_warning"] = query.value("admin_warning").toString();
        msgObj["send_time"] = query.value("send_time").toString();
        msgObj["status"] = query.value("status").toInt();
        msgArray.append(msgObj);
    }

    return msgArray;
}

// 11. 标记消息为已读
bool Database::markSellerMsgAsRead(const QString &msgId)
{
    QSqlQuery query;
    QString updateSql = "UPDATE seller_messages SET status = ? WHERE msg_id = ?";
    query.prepare(updateSql);
    query.bindValue(0, MSG_READ);
    query.bindValue(1, msgId);

    if (!query.exec()) {
        qDebug() << "标记卖家消息为已读失败：" << query.lastError().text();
        return false;
    }

    return true;
}
