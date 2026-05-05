#ifndef SELLERMSGWINDOW_H
#define SELLERMSGWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>

#include "tcpclient.h"

// 卖家通知页面：显示未读申诉结果通知（seller_messages 表）
// action:
//   - get_unread_seller_msg {seller_id}
//   - mark_seller_msg_read {msg_id}
class SellerMsgWindow : public QWidget
{
    Q_OBJECT
public:
    explicit SellerMsgWindow(int sellerId, TcpClient *tcpClient, QWidget *parent = nullptr);
    ~SellerMsgWindow() override = default;

    void setSellerId(int sellerId);

public slots:
    void refresh();

private slots:
    void onRefreshClicked();
    void onMarkReadClicked();

private:
    int m_sellerId = -1;
    TcpClient *m_tcpClient = nullptr;

    QLabel *m_title = nullptr;
    QLabel *m_tip = nullptr;
    QTableWidget *m_table = nullptr;
    QPushButton *m_btnRefresh = nullptr;
    QPushButton *m_btnMarkRead = nullptr;

    void initUI();
    void loadUnread();
    void fillTable(const QJsonArray &msgs);
};

#endif // SELLERMSGWINDOW_H
