#ifndef BUYHISTORYWINDOW_H
#define BUYHISTORYWINDOW_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QHash>
#include <QPixmap>

#include "tcpclient.h"

// 历史购买记录（卡片样式）
class BuyHistoryWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BuyHistoryWindow(int buyerId, TcpClient *tcpClient, QWidget *parent = nullptr);
    ~BuyHistoryWindow() override = default;

    void setBuyerId(int buyerId);

public slots:
    void refresh();

private slots:
    void onRefreshClicked();

private:
    int m_buyerId = -1;
    TcpClient *m_tcpClient = nullptr;

    QLabel *m_title = nullptr;
    QLabel *m_tip = nullptr;
    QListWidget *m_list = nullptr;
    QPushButton *m_btnRefresh = nullptr;

    // 图片缓存：避免重复拉取
    QHash<QString, QPixmap> m_coverCache;

    void initUI();
    void loadHistory();
    void fillList(const QJsonArray &books);

    QWidget* makeCardWidget(const QJsonObject &book);
    QPixmap fetchCoverPixmap(const QString &imagePath);
};

#endif // BUYHISTORYWINDOW_H
