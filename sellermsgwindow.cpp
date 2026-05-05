#include "sellermsgwindow.h"
#include <QHeaderView>
#include <QJsonObject>

SellerMsgWindow::SellerMsgWindow(int sellerId, TcpClient *tcpClient, QWidget *parent)
    : QWidget(parent), m_sellerId(sellerId), m_tcpClient(tcpClient)
{
    initUI();
}

void SellerMsgWindow::setSellerId(int sellerId)
{
    m_sellerId = sellerId;
}

void SellerMsgWindow::refresh()
{
    loadUnread();
}

void SellerMsgWindow::initUI()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);

    m_title = new QLabel("卖家通知（未读）");
    m_title->setStyleSheet("font-size:16px; font-weight:700;");
    root->addWidget(m_title);

    m_tip = new QLabel("这里显示你作为卖家的未读申诉处理结果通知（点击“标记已读”后将不再出现）。");
    m_tip->setStyleSheet("color:#666;");
    m_tip->setWordWrap(true);
    root->addWidget(m_tip);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"时间", "申诉ID", "管理员警告/处理意见", "申诉内容"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_table, 1);

    auto *row = new QHBoxLayout;
    row->addStretch();
    m_btnMarkRead = new QPushButton("标记已读");
    m_btnMarkRead->setObjectName("GhostBtn");
    m_btnRefresh = new QPushButton("刷新");
    m_btnRefresh->setObjectName("GhostBtn");
    row->addWidget(m_btnMarkRead);
    row->addWidget(m_btnRefresh);
    root->addLayout(row);

    connect(m_btnRefresh, &QPushButton::clicked, this, &SellerMsgWindow::onRefreshClicked);
    connect(m_btnMarkRead, &QPushButton::clicked, this, &SellerMsgWindow::onMarkReadClicked);
}

void SellerMsgWindow::onRefreshClicked()
{
    loadUnread();
}

void SellerMsgWindow::loadUnread()
{
    if (!m_tcpClient) return;
    if (m_sellerId <= 0) {
        m_tip->setStyleSheet("color:#b00020; font-weight:600;");
        m_tip->setText("无法加载：seller_id 无效（未登录或未同步用户ID）。");
        m_table->setRowCount(0);
        return;
    }

    QJsonObject req;
    req["action"] = "get_unread_seller_msg";
    req["seller_id"] = m_sellerId;

    QJsonObject resp = m_tcpClient->sendRequest(req);

    // SellerNotifyManager 返回 code=0，并且 unread_msgs/unread_count
    bool ok = (resp.contains("code") && resp.value("code").toInt() == 0)
              || (resp.value("status").toString() == "success");

    if (!ok) {
        m_tip->setStyleSheet("color:#b00020; font-weight:600;");
        m_tip->setText("加载失败：" + resp.value("message").toString("服务器未响应/接口未实现"));
        m_table->setRowCount(0);
        return;
    }

    QJsonArray msgs = resp.value("unread_msgs").toArray();
    m_tip->setStyleSheet("color:#666;");
    m_tip->setText(QString("未读通知：%1 条").arg(msgs.size()));
    fillTable(msgs);
}

void SellerMsgWindow::fillTable(const QJsonArray &msgs)
{
    m_table->setRowCount(0);
    m_table->setRowCount(msgs.size());

    for (int i = 0; i < msgs.size(); ++i) {
        QJsonObject m = msgs.at(i).toObject();

        auto *it0 = new QTableWidgetItem(m.value("send_time").toString());
        auto *it1 = new QTableWidgetItem(m.value("appeal_id").toString());
        auto *it2 = new QTableWidgetItem(m.value("admin_warning").toString());
        auto *it3 = new QTableWidgetItem(m.value("appeal_content").toString());

        // 把 msg_id 藏在第一列 item 的 data 里，标记已读要用
        it0->setData(Qt::UserRole, m.value("msg_id").toString());

        m_table->setItem(i, 0, it0);
        m_table->setItem(i, 1, it1);
        m_table->setItem(i, 2, it2);
        m_table->setItem(i, 3, it3);
    }
}

void SellerMsgWindow::onMarkReadClicked()
{
    if (!m_tcpClient) return;
    auto *it = m_table->currentItem();
    if (!it) return;

    // currentItem 可能不是第0列，确保拿到该行第0列
    int row = it->row();
    auto *it0 = m_table->item(row, 0);
    if (!it0) return;

    QString msgId = it0->data(Qt::UserRole).toString();
    if (msgId.isEmpty()) return;

    QJsonObject req;
    req["action"] = "mark_seller_msg_read";
    req["msg_id"] = msgId;

    QJsonObject resp = m_tcpClient->sendRequest(req);
    bool ok = (resp.contains("code") && resp.value("code").toInt() == 0)
              || (resp.value("status").toString() == "success");

    if (!ok) {
        m_tip->setStyleSheet("color:#b00020; font-weight:600;");
        m_tip->setText("标记失败：" + resp.value("message").toString("未知错误"));
        return;
    }

    loadUnread();
}
