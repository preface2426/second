#include "buyhistorywindow.h"

#include <QHeaderView>
#include <QJsonObject>
#include <QJsonDocument>
#include <QGraphicsDropShadowEffect>
#include <QBuffer>

BuyHistoryWindow::BuyHistoryWindow(int buyerId, TcpClient *tcpClient, QWidget *parent)
    : QWidget(parent), m_buyerId(buyerId), m_tcpClient(tcpClient)
{
    initUI();
}

void BuyHistoryWindow::setBuyerId(int buyerId)
{
    m_buyerId = buyerId;
}

void BuyHistoryWindow::refresh()
{
    loadHistory();
}

void BuyHistoryWindow::initUI()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);

    m_title = new QLabel("历史购买记录");
    m_title->setStyleSheet("font-size:16px; font-weight:700;");
    root->addWidget(m_title);

    m_tip = new QLabel("这里显示你已经购买成功的书（is_sold=1 且 buyer_id=你）。");
    m_tip->setStyleSheet("color:#666;");
    m_tip->setWordWrap(true);
    root->addWidget(m_tip);

    m_list = new QListWidget(this);
    m_list->setSpacing(8);
    m_list->setUniformItemSizes(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_list, 1);

    auto *row = new QHBoxLayout;
    row->addStretch();
    m_btnRefresh = new QPushButton("刷新");
    m_btnRefresh->setObjectName("GhostBtn");
    row->addWidget(m_btnRefresh);
    root->addLayout(row);

    connect(m_btnRefresh, &QPushButton::clicked, this, &BuyHistoryWindow::onRefreshClicked);
}

void BuyHistoryWindow::onRefreshClicked()
{
    loadHistory();
}

void BuyHistoryWindow::loadHistory()
{
    if (!m_tcpClient) return;
    if (m_buyerId <= 0) {
        m_tip->setStyleSheet("color:#b00020; font-weight:600;");
        m_tip->setText("无法加载：buyer_id 无效（未登录或未同步用户ID）。");
        m_list->clear();
        return;
    }

    QJsonObject req;
    req["action"] = "buy_history";
    req["buyer_id"] = m_buyerId;

    QJsonObject resp = m_tcpClient->sendRequest(req);

    bool ok = (resp.value("status").toString() == "success")
              || (resp.contains("code") && resp.value("code").toInt() == 0)
              || resp.contains("books");

    if (!ok) {
        m_tip->setStyleSheet("color:#b00020; font-weight:600;");
        m_tip->setText("加载失败：" + resp.value("message").toString("服务器未响应/接口未实现"));
        m_list->clear();
        return;
    }

    QJsonArray books = resp.value("books").toArray();

    m_tip->setStyleSheet("color:#666;");
    m_tip->setText(QString("已加载：%1 条购买记录").arg(books.size()));
    fillList(books);
}

void BuyHistoryWindow::fillList(const QJsonArray &books)
{
    m_list->clear();

    for (auto v : books) {
        QJsonObject b = v.toObject();

        QWidget *row = makeCardWidget(b);

        auto *item = new QListWidgetItem(m_list);
        item->setSizeHint(QSize(0, 102)); // 跟“我的”常见行高接近
        m_list->addItem(item);
        m_list->setItemWidget(item, row);
    }
}

QWidget* BuyHistoryWindow::makeCardWidget(const QJsonObject &book)
{
    // 外层卡片容器
    auto *card = new QFrame;
    card->setObjectName("HistoryCard");
    card->setStyleSheet(
        "QFrame#HistoryCard{"
        "background:#ffffff;"
        "border:1px solid #e6e6e6;"
        "border-radius:14px;"
        "}"
    );

    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 3);
    shadow->setColor(QColor(0,0,0,40));
    card->setGraphicsEffect(shadow);

    auto *hl = new QHBoxLayout(card);
    hl->setContentsMargins(14, 10, 14, 10);
    hl->setSpacing(12);

    // 左侧封面
    auto *cover = new QLabel;
    cover->setFixedSize(72, 92);
    cover->setStyleSheet("background:#f6f6f6; border-radius:10px; border:1px solid #eee;");
    cover->setAlignment(Qt::AlignCenter);

    const QString imagePath = book.value("image_path").toString();
    if (!imagePath.isEmpty()) {
        QPixmap pix = fetchCoverPixmap(imagePath);
        if (!pix.isNull()) {
            cover->setPixmap(pix.scaled(72, 92, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            cover->setText("无图");
            cover->setStyleSheet(cover->styleSheet() + "color:#999;");
        }
    } else {
        cover->setText("无图");
        cover->setStyleSheet(cover->styleSheet() + "color:#999;");
    }

    hl->addWidget(cover);

    // 右侧信息区
    auto *info = new QWidget;
    auto *vl = new QVBoxLayout(info);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(6);

    const int bookId = book.value("id").toInt();
    const QString title = book.value("title").toString();
    const QString cat = book.value("category").toString();
    const double price = book.value("price").toDouble();
    const QString sellerName = book.value("seller_name").toString();
    const QString time = book.value("created_time").toString();

    auto *labTitle = new QLabel(title.isEmpty() ? "(未命名书籍)" : title);
    labTitle->setStyleSheet("font-size:14px; font-weight:700;");
    labTitle->setWordWrap(false);
    labTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *labSub = new QLabel(QString("分类：%1   卖家：%2").arg(cat, sellerName));
    labSub->setStyleSheet("color:#666;");

    auto *labMeta = new QLabel(QString("ID：%1   上架时间：%2").arg(bookId).arg(time));
    labMeta->setStyleSheet("color:#888; font-size:12px;");

    auto *labPrice = new QLabel(QString("¥ %1").arg(QString::number(price, 'f', 2)));
    labPrice->setStyleSheet("font-size:14px; font-weight:700;");

    vl->addWidget(labTitle);
    vl->addWidget(labSub);
    vl->addWidget(labMeta);

    // 底部一行：价格 + 已购标识
    auto *bottom = new QHBoxLayout;
    bottom->setContentsMargins(0, 0, 0, 0);

    auto *tag = new QLabel("已购买");
    tag->setStyleSheet(
        "padding:2px 10px;"
        "border-radius:10px;"
        "background:#e8f5e9;"
        "color:#1e7f55;"
        "font-weight:700;"
    );

    bottom->addWidget(labPrice);
    bottom->addStretch();
    bottom->addWidget(tag);
    vl->addLayout(bottom);

    hl->addWidget(info, 1);

    return card;
}

QPixmap BuyHistoryWindow::fetchCoverPixmap(const QString &imagePath)
{
    if (imagePath.isEmpty()) return QPixmap();

    if (m_coverCache.contains(imagePath)) {
        return m_coverCache.value(imagePath);
    }
    if (!m_tcpClient) return QPixmap();

    QJsonObject req;
    req["action"] = "get_image";
    req["image_path"] = imagePath;
    QJsonObject resp = m_tcpClient->sendRequest(req);

    if (resp.value("status").toString() != "success") {
        return QPixmap();
    }

    QByteArray bytes = QByteArray::fromBase64(resp.value("data_b64").toString().toUtf8());
    QPixmap pix;
    pix.loadFromData(bytes);
    if (!pix.isNull()) m_coverCache.insert(imagePath, pix);
    return pix;
}
