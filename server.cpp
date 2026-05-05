#include "server.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QFile>

Server::Server(QObject *parent) : QTcpServer(parent)
{
    m_db.init();
    m_sellerNotifyMgr = new SellerNotifyManager(&m_db, this);
    m_appealMgr = new AppealManager(&m_db, m_sellerNotifyMgr, this);
}

bool Server::start(quint16 port)
{
    return listen(QHostAddress::Any, port);
}

void Server::incomingConnection(qintptr handle)
{
    QTcpSocket *socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(handle)) {
        socket->deleteLater();
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Server::onDisconnected);

    m_buffers[socket] = QByteArray();
}

void Server::onDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    m_buffers.remove(socket);
    socket->deleteLater();
}

void Server::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray &buf = m_buffers[socket];
    buf += socket->readAll();

    // 以 '\n' 分隔 JSON
    while (true) {
        int idx = buf.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = buf.left(idx).trimmed();
        buf.remove(0, idx + 1);

        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            QJsonObject resp;
            resp["status"] = "fail";
            resp["message"] = "invalid json";
            sendJson(socket, resp);
            continue;
        }

        processRequest(socket, doc.object());
    }
}

void Server::sendJson(QTcpSocket *socket, const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray out = doc.toJson(QJsonDocument::Compact);
    out.append('\n');
    socket->write(out);
    socket->flush();
}

void Server::processRequest(QTcpSocket *socket, const QJsonObject &req)
{
    const QString action = req.value("action").toString();
    QJsonObject resp;

    if (action == "register") {
        bool ok = m_db.registerUser(req["username"].toString(),
                                    req["password"].toString(),
                                    req["alipay"].toString());
        resp["status"] = ok ? "success" : "fail";
        sendJson(socket, resp);
        return;
    }

    if (action == "login") {
        int id = m_db.login(req["username"].toString(), req["password"].toString());
        if (id > 0) { resp["status"] = "success"; resp["user_id"] = id; }
        else resp["status"] = "fail";
        sendJson(socket, resp);
        return;
    }

    if (action == "upload_image") {
        const QString filename = req.value("filename").toString();
        const QString b64 = req.value("data_b64").toString();

        if (b64.isEmpty()) {
            resp["status"] = "fail";
            resp["message"] = "empty data";
            sendJson(socket, resp);
            return;
        }

        QByteArray bytes = QByteArray::fromBase64(b64.toUtf8());
        if (bytes.isEmpty()) {
            resp["status"] = "fail";
            resp["message"] = "invalid base64";
            sendJson(socket, resp);
            return;
        }

        // 用内容 hash 生成文件名，避免重复
        QByteArray hash = QCryptographicHash::hash(bytes, QCryptographicHash::Md5);
        QString ext = QFileInfo(filename).suffix();
        if (ext.isEmpty()) ext = "jpg";
        const QString saveName = QString(hash.toHex()).left(16) + "." + ext;

        QDir dir(QCoreApplication::applicationDirPath());
        if (!dir.exists("images")) dir.mkdir("images");

        const QString relPath = "images/" + saveName;
        const QString absPath = dir.filePath(relPath);

        QFile f(absPath);
        if (!f.open(QIODevice::WriteOnly)) {
            resp["status"] = "fail";
            resp["message"] = "cannot write file";
            sendJson(socket, resp);
            return;
        }
        f.write(bytes);
        f.close();

        resp["status"] = "success";
        resp["image_path"] = relPath;
        sendJson(socket, resp);
        return;
    }

    if (action == "get_image") {
        const QString relPath = req.value("image_path").toString();

        if (relPath.isEmpty() || !relPath.startsWith("images/")) {
            resp["status"] = "fail";
            resp["message"] = "bad image_path";
            sendJson(socket, resp);
            return;
        }

        QDir dir(QCoreApplication::applicationDirPath());
        const QString absPath = dir.filePath(relPath);

        QFile f(absPath);
        if (!f.open(QIODevice::ReadOnly)) {
            resp["status"] = "fail";
            resp["message"] = "file not found";
            sendJson(socket, resp);
            return;
        }

        QByteArray bytes = f.readAll();
        f.close();

        resp["status"] = "success";
        resp["data_b64"] = QString::fromUtf8(bytes.toBase64());
        sendJson(socket, resp);
        return;
    }

    if (action == "post_book") {
        bool ok = m_db.addBook(req["title"].toString(),
                               req["category"].toString(),
                               req["price"].toDouble(),
                               req["seller_id"].toInt(),
                               req["image_path"].toString());
        resp["status"] = ok ? "success" : "fail";
        sendJson(socket, resp);
        return;
    }

    if (action == "list_books") {
        resp["books"] = m_db.listBooks();
        sendJson(socket, resp);
        return;
    }

    if (action == "search_books") {
        resp["books"] = m_db.searchBooks(req["keyword"].toString());
        sendJson(socket, resp);
        return;
    }

    if (action == "my_books") {
        resp["books"] = m_db.booksBySeller(req["seller_id"].toInt());
        sendJson(socket, resp);
        return;
    }

    if (action == "simulate_payment") {
        bool ok = m_db.markBookSold(req["book_id"].toInt(), req["buyer_id"].toInt());
        resp["status"] = ok ? "success" : "fail";
        sendJson(socket, resp);
        return;
    }

    /* ================= admin: book management ================= */
    if (action == "delete_book") {
        const int bookId = req.value("book_id").toInt();
        QString sellerAlipay;
        const bool ok = m_db.deleteBook(bookId, &sellerAlipay);

        QJsonObject r;
        r["status"] = ok ? "success" : "fail";
        r["code"] = ok ? 0 : -1;
        r["seller_alipay"] = sellerAlipay;
        r["message"] = ok ? "deleted" : "delete failed";
        sendJson(socket, r);
        return;
    }

    /* ================= admin: appeals ================= */
    if (action == "get_pending_appeals") {
        QJsonObject r = m_appealMgr->processGetPendingAppeals();
        if (!r.contains("status")) r["status"] = (r.value("code").toInt() == 0 ? "success" : "fail");
        sendJson(socket, r);
        return;
    }

    if (action == "get_appeal_detail") {
        const QString appealId = req.value("appeal_id").toString();
        QJsonObject r;
        if (appealId.trimmed().isEmpty()) {
            r["code"] = -1;
            r["status"] = "fail";
            r["message"] = "missing appeal_id";
        } else {
            r["code"] = 0;
            r["status"] = "success";
            r["appeal_detail"] = m_db.getAppealDetail(appealId);
        }
        sendJson(socket, r);
        return;
    }

    if (action == "handle_appeal") {
        QJsonObject r = m_appealMgr->processHandleAppeal(req);
        if (!r.contains("status")) r["status"] = (r.value("code").toInt() == 0 ? "success" : "fail");
        sendJson(socket, r);
        return;
    }

    /* ================= chat ================= */

    // ✅ 发送消息：兼容旧版(from_user/to_user) + 新版(from_id/peer_id)
    if (action == "chat_send") {
        int bookId = req.value("book_id").toInt();

        int fromUser = req.contains("from_user") ? req.value("from_user").toInt()
                                                 : req.value("from_id").toInt();
        int toUser   = req.contains("to_user") ? req.value("to_user").toInt()
                                               : req.value("peer_id").toInt();

        QString text = req.value("text").toString().trimmed();
        if (text.isEmpty()) {
            resp["status"] = "fail";
            resp["message"] = "empty text";
            sendJson(socket, resp);
            return;
        }
        if (bookId <= 0 || fromUser <= 0 || toUser <= 0) {
            resp["status"] = "fail";
            resp["message"] = "bad params";
            sendJson(socket, resp);
            return;
        }

        int msgId = m_db.addMessage(bookId, fromUser, toUser, text);
        if (msgId < 0) {
            resp["status"] = "fail";
            resp["message"] = "db error";
            sendJson(socket, resp);
            return;
        }

        resp["status"] = "success";
        resp["msg_id"] = msgId;
        sendJson(socket, resp);
        return;
    }

    // ✅ 拉取消息：兼容 after_id（旧）和 after_msg_id（新）
    if (action == "chat_fetch") {
        int userId = req.value("user_id").toInt();
        int peerId = req.value("peer_id").toInt();
        int bookId = req.value("book_id").toInt();

        int afterId = req.contains("after_id") ? req.value("after_id").toInt(0)
                                               : req.value("after_msg_id").toInt(0);

        QJsonArray raw = m_db.fetchMessages(userId, peerId, bookId, afterId);

        // ✅ 将数据库字段统一成客户端字段：msg_id/from_id/to_id/text/time
        QJsonArray msgs;
        int lastId = afterId;
        for (auto v : raw) {
            QJsonObject o = v.toObject();
            QJsonObject m;
            const int id = o.value("id").toInt();
            m["msg_id"]  = id;
            m["book_id"] = o.value("book_id").toInt();
            m["from_id"] = o.value("from").toInt();
            m["to_id"]   = o.value("to").toInt();
            m["text"]    = o.value("text").toString();
            m["time"]    = o.value("time").toString();
            msgs.append(m);
            lastId = qMax(lastId, id);
        }

        // ✅ 拉取后标记已读（清未读数）
        m_db.markMessagesRead(userId, peerId, bookId);

        resp["status"] = "success";
        resp["messages"] = msgs;
        resp["last_id"] = lastId;          // 兼容字段
        resp["last_msg_id"] = lastId;      // 新字段
        sendJson(socket, resp);
        return;
    }

    // ✅ 申诉：买家提交
    if (action == "submit_appeal") {
        QJsonObject r = m_appealMgr->processBuyerAppeal(req);
        if (!r.contains("status")) r["status"] = (r.value("code").toInt() == 0 ? "success" : "fail");
        sendJson(socket, r);
        return;
    }

    // ✅ 卖家通知：拉取未读
    if (action == "get_unread_seller_msg") {
        QJsonObject r = m_sellerNotifyMgr->processGetUnreadSellerMsg(req);
        if (!r.contains("status")) r["status"] = (r.value("code").toInt() == 0 ? "success" : "fail");
        sendJson(socket, r);
        return;
    }

    // ✅ 卖家通知：标记已读
    if (action == "mark_seller_msg_read") {
        QJsonObject r = m_sellerNotifyMgr->processMarkSellerMsgAsRead(req);
        if (!r.contains("status")) r["status"] = (r.value("code").toInt() == 0 ? "success" : "fail");
        sendJson(socket, r);
        return;
    }

    // ✅ 历史购买记录
    if (action == "buy_history") {
        QJsonObject r;
        r["status"] = "success";
        r["code"] = 0;
        r["books"] = m_db.booksByBuyer(req.value("buyer_id").toInt());
        sendJson(socket, r);
        return;
    }

    // ✅ 会话列表：兼容 chat_list（旧）和 chat_sessions（新）
    if (action == "chat_list" || action == "chat_sessions") {
        int userId = req.value("user_id").toInt();
        resp["status"] = "success";
        resp["sessions"] = m_db.listChatSessions(userId);
        sendJson(socket, resp);
        return;
    }

    // ✅ 最后必须兜底回包，千万别什么都不回
    QJsonObject r;
    r["status"] = "fail";
    r["code"] = -1;
    r["message"] = QString("unknown action: %1").arg(action);
    sendJson(socket, r);
}
