#include "mainwindow.h"
#include "clickablelabel.h"

#include "sellermsgwindow.h"
#include "appealsubmitwindow.h"

#include <QtWidgets>
#include <QBuffer>
#include <QDateTime>

#include <animatedbutton.h>
#include <paymanager.h>
#include "sellermsgwindow.h"
#include "buyhistorywindow.h"
#include "appealsubmitwindow.h"

static constexpr int ROLE_BOOK_ID    = Qt::UserRole;
static constexpr int ROLE_IS_SOLD    = Qt::UserRole + 1;
static constexpr int ROLE_TITLE      = Qt::UserRole + 2;
static constexpr int ROLE_PRICE      = Qt::UserRole + 3;
static constexpr int ROLE_SELLER_ID  = Qt::UserRole + 4;
static constexpr int ROLE_SELLER_NAME = Qt::UserRole + 5;
static constexpr int ROLE_IMAGE_PATH = Qt::UserRole + 10;

// chat session list roles
static constexpr int ROLE_CHAT_PEER   = Qt::UserRole;
static constexpr int ROLE_CHAT_BOOK   = Qt::UserRole + 1;
static constexpr int ROLE_CHAT_TITLE  = Qt::UserRole + 2;
static constexpr int ROLE_CHAT_UNREAD = Qt::UserRole + 3;

// ====== 下面这两个小工具函数在你原文件里就有（如果你本来就有，保持一致即可） ======
static inline bool respOk(const QJsonObject &resp)
{
    const QString st = resp.value("status").toString();
    if (st == "success") return true;
    if (resp.contains("code") && resp.value("code").toInt() == 0) return true;
    if (resp.value("ok").toBool(false)) return true;
    return false;
}
// =====================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_client(new TcpClient(this))
{
    resize(1200, 800);
    setMinimumSize(1000, 700);

    if (!m_client->connectToServer()) {
        QMessageBox::critical(this, "网络", "无法连接服务器！");
        qApp->quit();
        return;
    }
    createLoginUI();
}

/* ================= 登录/注册 UI ================= */
void MainWindow::createLoginUI()
{
    auto *bar = menuBar();
    bar->clear();
    bar->setVisible(true);

    auto *w = new QWidget(this);
    setCentralWidget(w);

    auto *fl = new QFormLayout(w);
    fl->setContentsMargins(40, 30, 40, 30);

    m_leUser = new QLineEdit;
    m_lePwd  = new QLineEdit;
    m_lePwd->setEchoMode(QLineEdit::Password);
    m_leAlipay = new QLineEdit;

    fl->addRow("用户名:", m_leUser);
    fl->addRow("密  码:", m_lePwd);
    fl->addRow("支付宝沙盒账户:", m_leAlipay);

    auto *hl = new QHBoxLayout;
    auto *btnLogin = new AnimatedButton("登录");
    auto *btnReg   = new AnimatedButton("注册");
    btnLogin->setObjectName("PrimaryBtn");
    btnReg->setObjectName("GhostBtn");
    hl->addWidget(btnLogin);
    hl->addWidget(btnReg);
    fl->addRow(hl);

    connect(btnLogin, &QPushButton::clicked, this, &MainWindow::slotLogin);
    connect(btnReg,   &QPushButton::clicked, this, &MainWindow::slotRegister);
}

/* ================= 底部导航高亮同步 ================= */
void MainWindow::setNavChecked(int pageIndex)
{
    if (!m_navBrowseBtn) return;

    switch (pageIndex) {
    case 0: m_navBrowseBtn->setChecked(true); break;
    case 1: m_navSearchBtn->setChecked(true); break;
    case 2: m_navMineBtn->setChecked(true); break;
    case 3: m_navPostBtn->setChecked(true); break;
    case 4: m_navCartBtn->setChecked(true); break;
    case 5: m_navChatBtn->setChecked(true); break;

    case 6: if (m_navSellerMsgBtn) m_navSellerMsgBtn->setChecked(true); break;
    case 7: if (m_navHistoryBtn) m_navHistoryBtn->setChecked(true); break;
    case 8: if (m_navAppealBtn) m_navAppealBtn->setChecked(true); break;
    default: break;
    }
}


/* ================= 主界面 UI（Stack + BottomNav） ================= */
void MainWindow::createMainUI()
{
    // 不再用左上角菜单切换
    menuBar()->clear();
    menuBar()->setVisible(false);

    auto *w  = new QWidget;
    setCentralWidget(w);

    auto *root = new QVBoxLayout(w);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_stack = new QStackedWidget;
    root->addWidget(m_stack, 1);

    // ===== 0 浏览页 =====
    auto *browseW = new QWidget;
    auto *blay = new QVBoxLayout(browseW);

    auto *bTop = new QHBoxLayout;
    auto *btnChatSeller1 = new QPushButton("聊天");
    btnChatSeller1->setObjectName("GhostBtn");
    auto *btnAddCart1 = new QPushButton("加入购物车");
    btnAddCart1->setObjectName("PrimaryBtn");
    bTop->addWidget(btnChatSeller1);
    bTop->addWidget(btnAddCart1);
    bTop->addStretch();
    blay->addLayout(bTop);

    m_listAll = new QListWidget;
    m_listAll->setSpacing(8);
    blay->addWidget(m_listAll);

    m_stack->addWidget(browseW);

    connect(btnChatSeller1, &QPushButton::clicked, this, &MainWindow::slotChatWithSelectedBook);
    connect(btnAddCart1, &QPushButton::clicked, this, [this]{
        if (m_listAll && m_listAll->currentItem()) slotAddToCart(m_listAll->currentItem());
    });

    /* 1 搜索页 */
    auto *searchW = new QWidget;
    auto *slay = new QVBoxLayout(searchW);

    auto *hl1 = new QHBoxLayout;
    m_leKeyword = new QLineEdit;
    hl1->addWidget(new QLabel("关键字:"));
    hl1->addWidget(m_leKeyword);

    auto *btnChatSeller2 = new QPushButton("聊天");
    btnChatSeller2->setObjectName("GhostBtn");
    hl1->addWidget(btnChatSeller2);

    connect(btnChatSeller2, &QPushButton::clicked, this, &MainWindow::slotChatWithSelectedBook);

    auto *btnSearch = new AnimatedButton("搜索");
    btnSearch->setObjectName("GhostBtn");
    hl1->addWidget(btnSearch);
    slay->addLayout(hl1);

    m_listSearch = new QListWidget;
    m_listSearch->setSpacing(8);
    slay->addWidget(m_listSearch);

    m_stack->addWidget(searchW);
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::slotSearch);

    /* 2 我的书籍 */
    m_listMine = new QListWidget;
    m_listMine->setSpacing(8);
    m_stack->addWidget(m_listMine);

    /* 3 发布书籍 */
    auto *postW = new QWidget;
    auto *play = new QFormLayout(postW);

    m_leTitle  = new QLineEdit;
    m_leCategory = new QLineEdit;
    m_lePrice  = new QLineEdit;
    m_lePrice->setValidator(new QDoubleValidator(0, 999999, 2, this));

    auto *pic = new ClickableLabel("点击选择图片");
    pic->setAlignment(Qt::AlignCenter);
    pic->setMinimumHeight(220);
    pic->setStyleSheet("border:1px solid #bbb; border-radius:10px;");
    pic->setScaledContents(true);
    m_labPic = pic;

    play->addRow("书名:", m_leTitle);
    play->addRow("分类:", m_leCategory);
    play->addRow("价格:", m_lePrice);
    play->addRow("图片:", m_labPic);

    auto *btnPost = new AnimatedButton("发布");
    btnPost->setObjectName("PrimaryBtn");
    play->addRow(btnPost);
    m_stack->addWidget(postW);

    connect(pic, &ClickableLabel::clicked, this, [this]{
        QString f = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!f.isEmpty()) {
            m_localImagePath = f;
            QPixmap pix(f);
            m_labPic->setPixmap(pix.scaled(280, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    });
    connect(btnPost, &QPushButton::clicked, this, &MainWindow::slotPostBook);

    /* 4 购物车页 */
    auto *cartW = new QWidget;
    auto *clay = new QVBoxLayout(cartW);

    m_listCart = new QListWidget;
    m_listCart->setSpacing(8);
    clay->addWidget(m_listCart);

    auto *btnRow = new QHBoxLayout;
    auto *btnRemove = new AnimatedButton("移除选中");
    btnRemove->setObjectName("GhostBtn");
    auto *btnClear = new AnimatedButton("清空购物车");
    btnClear->setObjectName("GhostBtn");
    auto *btnCheckout = new AnimatedButton("结账");
    btnCheckout->setObjectName("PrimaryBtn");

    btnRow->addWidget(btnRemove);
    btnRow->addWidget(btnClear);
    btnRow->addStretch();
    btnRow->addWidget(btnCheckout);
    clay->addLayout(btnRow);

    m_labTotal = new QLabel("总价：0.00 元");
    m_labTotal->setAlignment(Qt::AlignRight);
    clay->addWidget(m_labTotal);

    m_stack->addWidget(cartW);

    connect(btnRemove, &QPushButton::clicked, this, &MainWindow::slotRemoveFromCart);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::slotClearCart);
    connect(btnCheckout, &QPushButton::clicked, this, &MainWindow::slotCheckout);

    /* 5 聊天页：左会话列表 + 右聊天 */
    auto *chatW = new QWidget;
    auto *chatRoot = new QHBoxLayout(chatW);

    m_chatSessionList = new QListWidget;
    m_chatSessionList->setFixedWidth(340);
    m_chatSessionList->setSpacing(6);
    chatRoot->addWidget(m_chatSessionList);

    auto *right = new QWidget;
    auto *rLay = new QVBoxLayout(right);

    m_chatInfo = new QLabel("未选择会话：点左侧历史聊天用户进入聊天");
    m_chatInfo->setWordWrap(true);
    m_chatInfo->setStyleSheet("font-weight:700;");
    rLay->addWidget(m_chatInfo);

    m_chatView = new QTextBrowser;
    m_chatView->setStyleSheet("background:#fff;border:1px solid #ddd;border-radius:10px;");
    rLay->addWidget(m_chatView, 1);

    auto *inputRow = new QHBoxLayout;
    m_chatInput = new QLineEdit;
    m_chatSendBtn = new QPushButton("发送");
    m_chatSendBtn->setObjectName("PrimaryBtn");
    inputRow->addWidget(m_chatInput, 1);
    inputRow->addWidget(m_chatSendBtn);
    rLay->addLayout(inputRow);

    chatRoot->addWidget(right, 1);
    m_stack->addWidget(chatW);

    /* 6 卖家通知页（申诉结果/管理员警告） */
    m_sellerMsgPage = new SellerMsgWindow(m_userId, m_client);
    m_stack->addWidget(m_sellerMsgPage);

    // 7 历史购买记录（BuyHistoryWindow）
    m_buyHistoryPage = new BuyHistoryWindow(m_userId, m_client);
    m_stack->addWidget(m_buyHistoryPage);

    // 8 申诉提交（AppealSubmitWindow）
    m_appealPage = new AppealSubmitWindow(m_userId, m_client);
    m_stack->addWidget(m_appealPage);


    connect(m_chatSendBtn, &QPushButton::clicked, this, &MainWindow::slotChatSend);
    connect(m_chatInput, &QLineEdit::returnPressed, this, &MainWindow::slotChatSend);

    connect(m_chatSessionList, &QListWidget::itemClicked, this, [this](QListWidgetItem *it){
        int peerId = it->data(ROLE_CHAT_PEER).toInt();
        int bookId = it->data(ROLE_CHAT_BOOK).toInt();
        QString title = it->data(ROLE_CHAT_TITLE).toString();
        enterChat(peerId, bookId, title);
    });

    // chat timer (poll)
    m_chatTimer = new QTimer(this);
    m_chatTimer->setInterval(1000);
    connect(m_chatTimer, &QTimer::timeout, this, &MainWindow::slotChatTick);
    m_chatTimer->start();

    // 双击：加入购物车（浏览 & 搜索）
    connect(m_listAll, &QListWidget::itemDoubleClicked, this, &MainWindow::slotAddToCart);
    connect(m_listSearch, &QListWidget::itemDoubleClicked, this, &MainWindow::slotAddToCart);

    // hover preview
    setupHoverPreview();

    // ===== 底部导航栏 =====
    auto *navBar = new QWidget;
    navBar->setObjectName("BottomNav");
    navBar->setFixedHeight(76);

    auto *navLay = new QHBoxLayout(navBar);
    navLay->setContentsMargins(12, 10, 12, 12);
    navLay->setSpacing(10);

    auto mkNavBtn = [&](const QString &text){
        auto *b = new QPushButton(text);
        b->setObjectName("NavBtn");
        b->setCheckable(true);
        b->setAutoExclusive(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(48);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        return b;
    };

    m_navBrowseBtn = mkNavBtn("浏览");
    m_navSearchBtn = mkNavBtn("搜索");
    m_navPostBtn   = mkNavBtn("发布");
    m_navCartBtn   = mkNavBtn("购物车");
    m_navChatBtn   = mkNavBtn("聊天");
    m_navMineBtn   = mkNavBtn("我的");

    m_navSellerMsgBtn = mkNavBtn("卖家通知");
    m_navHistoryBtn = mkNavBtn("历史购买");
    m_navAppealBtn = mkNavBtn("申诉");

    navLay->addWidget(m_navBrowseBtn);
    navLay->addWidget(m_navSearchBtn);
    navLay->addWidget(m_navPostBtn);
    navLay->addWidget(m_navCartBtn);
    navLay->addWidget(m_navChatBtn);
    navLay->addWidget(m_navMineBtn);
    navLay->addWidget(m_navSellerMsgBtn);
    navLay->addWidget(m_navHistoryBtn);
    navLay->addWidget(m_navAppealBtn);

    root->addWidget(navBar);

    connect(m_navBrowseBtn, &QPushButton::clicked, this, &MainWindow::slotBrowse);
    connect(m_navSearchBtn, &QPushButton::clicked, this, [this]{ hideHoverPreview(); m_stack->setCurrentIndex(1); setNavChecked(1); });
    connect(m_navMineBtn,   &QPushButton::clicked, this, &MainWindow::slotMyBooks);
    connect(m_navPostBtn,   &QPushButton::clicked, this, [this]{ hideHoverPreview(); m_stack->setCurrentIndex(3); setNavChecked(3); });
    connect(m_navCartBtn,   &QPushButton::clicked, this, [this]{ hideHoverPreview(); m_stack->setCurrentIndex(4); setNavChecked(4); });
    connect(m_navChatBtn,   &QPushButton::clicked, this, &MainWindow::slotOpenChat);

    connect(m_navSellerMsgBtn, &QPushButton::clicked, this, &MainWindow::slotOpenSellerMsg);
    connect(m_navHistoryBtn, &QPushButton::clicked, this, &MainWindow::slotOpenBuyHistory);
    connect(m_navAppealBtn, &QPushButton::clicked, this, &MainWindow::slotOpenAppealSubmit);


    // 默认选中浏览
    m_navBrowseBtn->setChecked(true);
}

/* ================= Seller notify / Appeal pages ================= */
void MainWindow::slotOpenSellerMsg()
{
    hideHoverPreview();
    m_stack->setCurrentIndex(6);
    setNavChecked(6);
    if (m_sellerMsgPage) { m_sellerMsgPage->setSellerId(m_userId); m_sellerMsgPage->refresh(); }
}

void MainWindow::slotOpenBuyHistory()
{
    hideHoverPreview();
    m_stack->setCurrentIndex(7);
    setNavChecked(7);
    if (m_buyHistoryPage) { m_buyHistoryPage->setBuyerId(m_userId); m_buyHistoryPage->refresh(); }
}



void MainWindow::slotOpenAppealSubmit()
{
    setNavChecked(8);
    hideHoverPreview();
    m_stack->setCurrentIndex(8);
    if (m_appealPage) {
        m_appealPage->refresh();
    }
}

/* ================= 图片拉取与缓存 ================= */
QPixmap MainWindow::fetchCoverPixmap(const QString &imagePath)
{
    if (imagePath.isEmpty()) return {};

    if (m_coverCache.contains(imagePath)) {
        return m_coverCache.value(imagePath);
    }

    QJsonObject req{{"action","get_image"}, {"image_path", imagePath}};
    QJsonObject resp = m_client->sendRequest(req);
    if (resp.value("status").toString() != "success") return {};

    QByteArray bytes = QByteArray::fromBase64(resp.value("data_b64").toString().toUtf8());
    QPixmap pix;
    if (!pix.loadFromData(bytes)) return {};

    m_coverCache.insert(imagePath, pix);
    return pix;
}

/* ================= 生成书籍条目 widget ================= */
QWidget* MainWindow::makeBookRowWidget(const QJsonObject &book, int &outBookId)
{
    outBookId = book.value("id").toInt();

    const QString title = book.value("title").toString();
    const double price = book.value("price").toDouble();
    const bool sold = book.value("is_sold").toInt() != 0;
    const QString imagePath = book.value("image_path").toString();

    auto *row = new QWidget;
    auto *hl = new QHBoxLayout(row);
    hl->setContentsMargins(10, 8, 10, 8);
    hl->setSpacing(12);

    auto *cover = new QLabel;
    cover->setFixedSize(60, 80);
    cover->setAlignment(Qt::AlignCenter);
    cover->setStyleSheet("background:#eef1f6; border-radius:10px; color:#667; font-weight:700;");

    QPixmap pix = fetchCoverPixmap(imagePath);
    if (!pix.isNull()) {
        cover->setPixmap(pix.scaled(60, 80, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        cover->setStyleSheet("border-radius:10px;");
    } else {
        cover->setText("NO\nIMG");
    }

    auto *vl = new QVBoxLayout;
    vl->setSpacing(4);

    auto *labTitle = new QLabel(title);
    labTitle->setStyleSheet("font-size:16px; font-weight:700;");
    labTitle->setWordWrap(true);

    auto *labPrice = new QLabel(QString("￥%1").arg(price, 0, 'f', 2));
    labPrice->setStyleSheet("font-size:14px; font-weight:700; color:#4c5fff;");

    auto *labStatus = new QLabel(sold ? "已售出" : "在售（双击加入购物车）");
    labStatus->setStyleSheet(sold ? "color:#888;" : "color:#1e7f55;");

    vl->addWidget(labTitle);
    vl->addWidget(labPrice);
    vl->addWidget(labStatus);

    hl->addWidget(cover);
    hl->addLayout(vl, 1);

    row->setStyleSheet("background:white; border:1px solid #e7e8ee; border-radius:14px;");
    return row;
}

/* ---------- auth ---------- */
void MainWindow::slotLogin()
{
    QJsonObject req{{"action","login"},
                    {"username",m_leUser->text()},
                    {"password",m_lePwd->text()}};
    auto resp = m_client->sendRequest(req);
    if (resp["status"].toString() == "success") {
        m_userId = resp["user_id"].toInt();
        createMainUI();
        slotBrowse();
    } else {
        QMessageBox::warning(this, "登录", "用户名或密码错误");
    }
}

void MainWindow::slotRegister()
{
    QJsonObject req{{"action","register"},
                    {"username",m_leUser->text()},
                    {"password",m_lePwd->text()},
                    {"alipay",m_leAlipay->text()}};
    auto resp = m_client->sendRequest(req);
    QMessageBox::information(this, "注册", resp["status"].toString()=="success"?"注册成功":"注册失败");
}

/* ---------- browse/search/mine ---------- */
void MainWindow::slotBrowse()
{
    setNavChecked(0);
    hideHoverPreview();
    m_stack->setCurrentIndex(0);

    QJsonObject req{{"action","list_books"}};
    auto arr = m_client->sendRequest(req)["books"].toArray();

    m_listAll->clear();
    for (auto v : arr) {
        QJsonObject o = v.toObject();
        int bookId = -1;
        QWidget *row = makeBookRowWidget(o, bookId);

        auto *item = new QListWidgetItem(m_listAll);
        item->setSizeHint(QSize(0, 102));
        item->setData(ROLE_BOOK_ID, bookId);
        item->setData(ROLE_IS_SOLD, o.value("is_sold").toInt());
        item->setData(ROLE_TITLE, o.value("title").toString());
        item->setData(ROLE_PRICE, o.value("price").toDouble());
        item->setData(ROLE_SELLER_ID, o.value("seller_id").toInt());
        item->setData(ROLE_IMAGE_PATH, o.value("image_path").toString());
        item->setData(ROLE_SELLER_NAME, o.value("seller_name").toString());
        m_listAll->addItem(item);
        m_listAll->setItemWidget(item, row);
    }
}

void MainWindow::slotSearch()
{
    setNavChecked(1);
    hideHoverPreview();
    m_stack->setCurrentIndex(1);

    m_listSearch->clear();
    QJsonObject req{{"action","search_books"},{"keyword",m_leKeyword->text()}};
    auto arr = m_client->sendRequest(req)["books"].toArray();

    for (auto v : arr) {
        QJsonObject o = v.toObject();
        int bookId = -1;
        QWidget *row = makeBookRowWidget(o, bookId);

        auto *item = new QListWidgetItem(m_listSearch);
        item->setSizeHint(QSize(0, 102));
        item->setData(ROLE_BOOK_ID, bookId);
        item->setData(ROLE_IS_SOLD, o.value("is_sold").toInt());
        item->setData(ROLE_TITLE, o.value("title").toString());
        item->setData(ROLE_PRICE, o.value("price").toDouble());
        item->setData(ROLE_SELLER_ID, o.value("seller_id").toInt());
        item->setData(ROLE_IMAGE_PATH, o.value("image_path").toString());
        item->setData(ROLE_SELLER_NAME, o.value("seller_name").toString());
        m_listSearch->addItem(item);
        m_listSearch->setItemWidget(item, row);
    }
}

void MainWindow::slotMyBooks()
{
    setNavChecked(2);
    hideHoverPreview();
    m_stack->setCurrentIndex(2);

    QJsonObject req{{"action","my_books"},{"seller_id",m_userId}};
    auto arr = m_client->sendRequest(req)["books"].toArray();

    m_listMine->clear();
    for (auto v : arr) {
        QJsonObject o = v.toObject();
        int bookId = -1;
        QWidget *row = makeBookRowWidget(o, bookId);

        auto *item = new QListWidgetItem(m_listMine);
        item->setSizeHint(QSize(0, 102));
        item->setData(ROLE_BOOK_ID, bookId);
        item->setData(ROLE_IS_SOLD, o.value("is_sold").toInt());
        item->setData(ROLE_TITLE, o.value("title").toString());
        item->setData(ROLE_PRICE, o.value("price").toDouble());
        item->setData(ROLE_SELLER_ID, o.value("seller_id").toInt());
        item->setData(ROLE_IMAGE_PATH, o.value("image_path").toString());
        item->setData(ROLE_SELLER_NAME, o.value("seller_name").toString());
        m_listMine->addItem(item);
        m_listMine->setItemWidget(item, row);
    }
}

/* ---------- post ---------- */
void MainWindow::slotPostBook()
{
    if (m_leTitle->text().isEmpty() || m_lePrice->text().isEmpty() || m_localImagePath.isEmpty()) {
        QMessageBox::warning(this, "发布", "请填写完整并选择图片");
        return;
    }

    QImage img(m_localImagePath);
    if (img.isNull()) {
        QMessageBox::warning(this, "发布", "图片读取失败");
        return;
    }
    QImage scaled = img;
    if (scaled.width() > 600) scaled = img.scaledToWidth(600, Qt::SmoothTransformation);

    QByteArray imgBytes;
    {
        QBuffer buffer(&imgBytes);
        buffer.open(QIODevice::WriteOnly);
        scaled.save(&buffer, "JPG", 85);
    }

    // 1) 上传图片
    QJsonObject upReq{
        {"action","upload_image"},
        {"filename", QFileInfo(m_localImagePath).fileName()},
        {"data_b64", QString::fromUtf8(imgBytes.toBase64())}
    };
    QJsonObject upResp = m_client->sendRequest(upReq);
    if (upResp.value("status").toString() != "success") {
        QMessageBox::warning(this, "发布", "图片上传失败：" + upResp.value("message").toString());
        return;
    }
    const QString serverImagePath = upResp.value("image_path").toString();

    // 2) 发布书籍
    QJsonObject req{
        {"action","post_book"},
        {"title",m_leTitle->text()},
        {"image_path",serverImagePath},
        {"category",m_leCategory->text()},
        {"price",m_lePrice->text().toDouble()},
        {"seller_id",m_userId}
    };
    auto resp = m_client->sendRequest(req);

    QMessageBox::information(this, "发布", resp["status"].toString()=="success"?"发布成功":"发布失败");

    if (resp["status"].toString() == "success") {
        m_leTitle->clear();
        m_leCategory->clear();
        m_lePrice->clear();
        m_labPic->clear();
        m_labPic->setText("点击选择图片");
        m_localImagePath.clear();
        slotBrowse();
    }
}


/* ================= cart ================= */
void MainWindow::refreshCartTotalLabel()
{
    if (m_labTotal) m_labTotal->setText(QString("总价：%1 元").arg(m_cartTotal, 0, 'f', 2));
}

void MainWindow::removeCartItemFromUI(int bookId)
{
    if (!m_listCart) return;
    for (int i = 0; i < m_listCart->count(); ++i) {
        auto *it = m_listCart->item(i);
        if (it && it->data(ROLE_BOOK_ID).toInt() == bookId) {
            delete m_listCart->takeItem(i);
            return;
        }
    }
}

void MainWindow::slotAddToCart(QListWidgetItem *item)
{
    if (!item) return;

    int bookId = item->data(ROLE_BOOK_ID).toInt();
    int isSold = item->data(ROLE_IS_SOLD).toInt();
    int sellerId = item->data(ROLE_SELLER_ID).toInt();
    QString sellerName = item->data(ROLE_SELLER_NAME).toString();
    QString title = item->data(ROLE_TITLE).toString();
    double price = item->data(ROLE_PRICE).toDouble();
    QString imagePath = item->data(ROLE_IMAGE_PATH).toString();

    if (isSold != 0) {
        QMessageBox::warning(this, "购物车", "这本书已经卖掉啦，不能加入购物车。");
        return;
    }
    if (m_cart.contains(bookId)) {
        statusBar()->showMessage("已在购物车中", 1200);
        return;
    }

    CartItem ci;
    ci.bookId = bookId;
    ci.sellerId = sellerId;
    ci.sellerName = sellerName;
    ci.title = title;
    ci.price = price;
    ci.isSold = isSold;
    ci.imagePath = imagePath;

    m_cart.insert(bookId, ci);
    m_cartTotal += price;

    if (m_listCart) {
        auto *cartItem = new QListWidgetItem(QString("《%1》  ￥%2  (id=%3)")
                                             .arg(title)
                                             .arg(price, 0, 'f', 2)
                                             .arg(bookId));
        cartItem->setSizeHint(QSize(0, 48));
        cartItem->setData(ROLE_BOOK_ID, bookId);
        cartItem->setData(ROLE_IS_SOLD, isSold);
        cartItem->setData(ROLE_SELLER_NAME, sellerName);
        cartItem->setData(ROLE_TITLE, title);
        cartItem->setData(ROLE_PRICE, price);
        cartItem->setData(ROLE_SELLER_ID, sellerId);
        cartItem->setData(ROLE_IMAGE_PATH, imagePath);
        m_listCart->addItem(cartItem);
    }

    refreshCartTotalLabel();
    statusBar()->showMessage("已加入购物车（可在底部点“购物车”查看）", 1600);
}

void MainWindow::slotRemoveFromCart()
{
    if (!m_listCart) return;
    auto *it = m_listCart->currentItem();
    if (!it) {
        QMessageBox::information(this, "购物车", "请先选中要移除的商品");
        return;
    }

    int bookId = it->data(ROLE_BOOK_ID).toInt();
    if (m_cart.contains(bookId)) {
        m_cartTotal -= m_cart[bookId].price;
        m_cart.remove(bookId);
    }
    delete m_listCart->takeItem(m_listCart->row(it));
    refreshCartTotalLabel();
}

void MainWindow::slotClearCart()
{
    hideHoverPreview();
    m_cart.clear();
    m_cartTotal = 0.0;
    if (m_listCart) m_listCart->clear();
    refreshCartTotalLabel();
}

void MainWindow::slotCheckout()
{
    if (m_cart.isEmpty()) {
        QMessageBox::information(this, "结账", "购物车为空。");
        return;
    }
    if (m_userId <= 0) {
        QMessageBox::warning(this, "结账", "请先登录。");
        return;
    }

    const double total = m_cartTotal;
    const QString orderNo = QString("CART%1").arg(QDateTime::currentMSecsSinceEpoch());

    auto *payMgr = new PayManager(this);

    connect(payMgr, &PayManager::errorOccurred, this, [this, payMgr](QString msg){
        QMessageBox::critical(this, "支付错误", msg);
        payMgr->deleteLater();
    });

    // urlReady 这里只做提示 + 释放 payMgr，不再在这里处理“置已售出”
    connect(payMgr, &PayManager::urlReady, this, [this, payMgr, orderNo, total](QString){
        statusBar()->showMessage(
            QString("支付页面已打开：订单号 %1，应付 %2 元")
                .arg(orderNo)
                .arg(total, 0, 'f', 2),
            4000
        );
        payMgr->deleteLater();
    });

    // 1) 发起支付（打开浏览器）
    payMgr->createPayment(orderNo, total, "CartBooks");

    // 2) ✅ 按你的要求：执行完 createPayment 之后，立刻把购物车商品标记为已售出
    int okCount = 0;
    QStringList failList;
    QList<int> removeIds;

    for (auto it = m_cart.begin(); it != m_cart.end(); ++it) {
        const CartItem &ci = it.value();

        // 这里调用 server.cpp 里的 simulate_payment（你说的 payment_sim）
        QJsonObject req{
            {"action", "simulate_payment"},
            {"book_id", ci.bookId},
            {"buyer_id", m_userId}
        };
        QJsonObject resp = m_client->sendRequest(req);

        if (resp.value("status").toString() == "success") {
            okCount++;
            removeIds.append(ci.bookId);
        } else {
            failList << QString("《%1》(id=%2)").arg(ci.title).arg(ci.bookId);
        }
    }

    // 3) 从购物车移除已成功置为“已售出”的条目
    for (int id : removeIds) {
        if (m_cart.contains(id)) {
            m_cartTotal -= m_cart[id].price;
            m_cart.remove(id);
            removeCartItemFromUI(id);
        }
    }
    refreshCartTotalLabel();

    // 4) 提示结果 + 刷新浏览页（让“已售出”立刻可见）
    QString msg = QString("已发起支付，并同步更新商品状态：成功 %1 本").arg(okCount);
    if (!failList.isEmpty()) {
        msg += "\n以下书籍购买失败（可能已售出/被抢购）：\n" + failList.join("\n");
    }
    QMessageBox::information(this, "结账结果", msg);

    slotBrowse();
}

/* ================= 悬停预览（图片 + 文字信息） ================= */
void MainWindow::setupHoverPreview()
{
    if (m_hoverTimer) return;

    m_hoverTimer = new QTimer(this);
    m_hoverTimer->setSingleShot(true);
    connect(m_hoverTimer, &QTimer::timeout, this, [this](){
        if (m_hoverList && m_hoverItem) {
            showHoverPreview(m_hoverList, m_hoverItem, m_hoverGlobalPos);
        }
    });

    m_hoverPopup = new QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint);
    m_hoverPopup->setAttribute(Qt::WA_ShowWithoutActivating);
    m_hoverPopup->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hoverPopup->setStyleSheet("QFrame{background:white;border:1px solid #aaa;border-radius:12px;}");

    auto *lay = new QVBoxLayout(m_hoverPopup);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    m_hoverImage = new QLabel;
    m_hoverImage->setFixedSize(560, 680);
    m_hoverImage->setAlignment(Qt::AlignCenter);
    m_hoverImage->setStyleSheet("background:#f3f3f3;border-radius:10px;");
    m_hoverImage->setScaledContents(true);

    m_hoverTitle = new QLabel;
    m_hoverTitle->setStyleSheet("font-weight:600;font-size:14px;color:#111;");
    m_hoverTitle->setWordWrap(true);

    m_hoverPrice = new QLabel;
    m_hoverPrice->setStyleSheet("color:#333;");

    m_hoverStatus = new QLabel;
    m_hoverStatus->setStyleSheet("color:#090;");

    m_hoverId = new QLabel;
    m_hoverId->setStyleSheet("color:#666;font-size:12px;");

    lay->addWidget(m_hoverImage);
    lay->addWidget(m_hoverTitle);
    lay->addWidget(m_hoverPrice);
    lay->addWidget(m_hoverStatus);
    lay->addWidget(m_hoverId);

    auto install = [this](QListWidget *list){
        if (!list) return;
        list->setMouseTracking(true);
        list->viewport()->setMouseTracking(true);
        list->viewport()->installEventFilter(this);
    };
    install(m_listAll);
    install(m_listSearch);
    install(m_listCart);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    QListWidget *list = nullptr;
    if (m_listAll && watched == m_listAll->viewport()) list = m_listAll;
    else if (m_listSearch && watched == m_listSearch->viewport()) list = m_listSearch;
    else if (m_listCart && watched == m_listCart->viewport()) list = m_listCart;

    if (!list) return QMainWindow::eventFilter(watched, event);

    if (event->type() == QEvent::Leave ||
        event->type() == QEvent::Wheel ||
        event->type() == QEvent::MouseButtonPress) {
        hideHoverPreview();
        return QMainWindow::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove) {
        auto *me = static_cast<QMouseEvent*>(event);
        QListWidgetItem *it = list->itemAt(me->pos());

        if (!it) {
            hideHoverPreview();
            return QMainWindow::eventFilter(watched, event);
        }

        if (it != m_hoverItem || list != m_hoverList) {
            hideHoverPreview();
            m_hoverList = list;
            m_hoverItem = it;
            m_hoverGlobalPos = me->globalPos();
            m_hoverTimer->start(1000); // 1s
        } else {
            m_hoverGlobalPos = me->globalPos();
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showHoverPreview(QListWidget *, QListWidgetItem *item, const QPoint &globalPos)
{
    if (!m_hoverPopup || !m_hoverImage || !item) return;

    const int bookId = item->data(ROLE_BOOK_ID).toInt();
    const int isSold = item->data(ROLE_IS_SOLD).toInt();
    const QString title = item->data(ROLE_TITLE).toString();
    const double price = item->data(ROLE_PRICE).toDouble();
    const QString imagePath = item->data(ROLE_IMAGE_PATH).toString();

    if (imagePath.isEmpty()) return;

    QPixmap cover = fetchCoverPixmap(imagePath);
    if (cover.isNull()) return;

    m_hoverImage->setPixmap(cover.scaled(m_hoverImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    m_hoverTitle->setText(QString("《%1》").arg(title.isEmpty() ? "未知书名" : title));
    m_hoverPrice->setText(QString("价格：￥%1").arg(price, 0, 'f', 2));
    m_hoverStatus->setText(isSold ? "状态：已售出" : "状态：在售");
    m_hoverStatus->setStyleSheet(isSold ? "color:#b00;" : "color:#090;");
    m_hoverId->setText(QString("ID：%1").arg(bookId));

    QPoint p = globalPos + QPoint(18, 18);
    m_hoverPopup->move(p);
    m_hoverPopup->show();
}

void MainWindow::hideHoverPreview()
{
    if (m_hoverTimer) m_hoverTimer->stop();
    if (m_hoverPopup) m_hoverPopup->hide();
    m_hoverList = nullptr;
    m_hoverItem = nullptr;
}

/* ================= chat =================
   说明：这里的 action 名称是“建议版”：
   - chat_sessions: 返回会话列表（peer_id, book_id, book_title, unread）
   - chat_fetch: 拉取消息（peer_id, book_id, after_msg_id）
   - chat_send: 发送消息（peer_id, book_id, from_id, text）
   如果服务端没实现，会提示“服务器未实现聊天接口”，但程序依然能跑。
*/
void MainWindow::slotOpenChat()
{
    setNavChecked(5);
    hideHoverPreview();
    m_stack->setCurrentIndex(5);
    refreshChatSessionList();
}

void MainWindow::refreshChatSessionList()
{
    if (m_userId <= 0 || !m_chatSessionList) return;

    QJsonObject req{{"action","chat_sessions"}, {"user_id", m_userId}};
    QJsonObject resp = m_client->sendRequest(req);

    const bool ok = respOk(resp) || resp.contains("sessions");
    if (!ok) {
        // 服务器可能还没实现聊天
        m_chatSessionList->clear();
        auto *it = new QListWidgetItem("（服务器未实现 chat_sessions）");
        it->setFlags(Qt::NoItemFlags);
        m_chatSessionList->addItem(it);
        return;
    }

    QJsonArray sessions = resp.value("sessions").toArray();
    m_chatSessionList->clear();

    if (sessions.isEmpty()) {
        auto *it = new QListWidgetItem("暂无会话（有人给你发消息后会出现在这里）");
        it->setFlags(Qt::NoItemFlags);
        m_chatSessionList->addItem(it);
        return;
    }

    for (auto v : sessions) {
        QJsonObject s = v.toObject();
        int peerId = s.value("peer_id").toInt();
        int bookId = s.value("book_id").toInt();
        QString title = s.value("book_title").toString();
        int unread = s.value("unread").toInt();

        QString text = QString("用户 %1\n《%2》").arg(peerId).arg(title.isEmpty() ? "-" : title);
        if (unread > 0) text += QString("\n未读：%1").arg(unread);

        auto *item = new QListWidgetItem(text);
        item->setData(ROLE_CHAT_PEER, peerId);
        item->setData(ROLE_CHAT_BOOK, bookId);
        item->setData(ROLE_CHAT_TITLE, title);
        item->setData(ROLE_CHAT_UNREAD, unread);

        if (unread > 0) item->setForeground(QBrush(QColor("#d12")));
        m_chatSessionList->addItem(item);
    }
}

void MainWindow::enterChat(int peerId, int bookId, const QString &bookTitle)
{
    m_chatPeerId = peerId;
    m_chatBookId = bookId;
    m_chatBookTitle = bookTitle;
    m_chatLastMsgId = 0;

    if (m_chatInfo) {
        m_chatInfo->setText(QString("正在聊天：对方用户 %1 ｜ 书籍《%2》")
                            .arg(peerId)
                            .arg(bookTitle.isEmpty() ? "-" : bookTitle));
    }
    if (m_chatView) m_chatView->clear();

    // 进入会话后立刻拉一次消息
    slotChatTick();
}

void MainWindow::appendChatMessage(const QJsonObject &msg)
{
    if (!m_chatView) return;
    int from = msg.value("from_id").toInt();
    QString text = msg.value("text").toString();
    QString ts = msg.value("time").toString();
    if (ts.isEmpty()) ts = QDateTime::currentDateTime().toString("MM-dd hh:mm:ss");

    QString who = (from == m_userId) ? "我" : QString("对方(%1)").arg(from);
    m_chatView->append(QString("[%1] %2：%3").arg(ts, who, text.toHtmlEscaped()));
}

void MainWindow::slotChatSend()
{
    if (m_userId <= 0) return;
    if (m_chatPeerId <= 0 || m_chatBookId <= 0) {
        QMessageBox::information(this, "聊天", "请先在左侧选择一个会话");
        return;
    }
    QString text = m_chatInput ? m_chatInput->text().trimmed() : "";
    if (text.isEmpty()) return;

    QJsonObject req{
        {"action", "chat_send"},
        {"from_id", m_userId},
        {"peer_id", m_chatPeerId},
        {"book_id", m_chatBookId},
        {"text", text}
    };
    QJsonObject resp = m_client->sendRequest(req);
    const bool ok = respOk(resp)
                    || resp.contains("msg_id")
                    || resp.contains("id")
                    || resp.value("ok").toBool(false);
    if (!ok) {
        QMessageBox::warning(this, "聊天", "发送失败（服务器未实现 chat_send 或网络异常）");
        return;
    }

    // 本地先显示一条
    QJsonObject fake{{"from_id", m_userId}, {"text", text}, {"time", QDateTime::currentDateTime().toString("MM-dd hh:mm:ss")}};
    appendChatMessage(fake);

    if (m_chatInput) m_chatInput->clear();
}

void MainWindow::slotChatTick()
{
    if (m_userId <= 0) return;

    // 1) 刷新左侧会话列表（有未读就显示出来）
    refreshChatSessionList();

    // 2) 如果当前在某个会话里，拉取新消息
    if (m_chatPeerId <= 0 || m_chatBookId <= 0) return;

    QJsonObject req{
        {"action","chat_fetch"},
        {"user_id", m_userId},
        {"peer_id", m_chatPeerId},
        {"book_id", m_chatBookId},
        {"after_msg_id", m_chatLastMsgId}
    };
    QJsonObject resp = m_client->sendRequest(req);
    const bool ok = respOk(resp) || resp.contains("messages");
    if (!ok) return;

    QJsonArray msgs = resp.value("messages").toArray();
    for (auto v : msgs) {
        QJsonObject m = v.toObject();
        int mid = m.value("msg_id").toInt();
        if (mid > m_chatLastMsgId) m_chatLastMsgId = mid;
        appendChatMessage(m);
    }
}
void MainWindow::slotChatWithSelectedBook()
{
    QListWidgetItem *item = nullptr;

    // 当前页判断：0 浏览 / 1 搜索
    int idx = m_stack ? m_stack->currentIndex() : 0;
    if (idx == 0 && m_listAll) item = m_listAll->currentItem();
    if (idx == 1 && m_listSearch) item = m_listSearch->currentItem();

    if (!item) {
        QMessageBox::information(this, "聊天", "请先在浏览/搜索列表里选中一本书。");
        return;
    }

    int bookId = item->data(ROLE_BOOK_ID).toInt();
    int sellerId = item->data(ROLE_SELLER_ID).toInt();
    QString title = item->data(ROLE_TITLE).toString();

    if (sellerId <= 0 || bookId <= 0) {
        QMessageBox::warning(this, "聊天", "当前条目缺少卖家/书籍信息。");
        return;
    }
    if (sellerId == m_userId) {
        QMessageBox::information(this, "聊天", "这是你自己的书，不需要私聊。");
        return;
    }

    // 切到聊天页
    if (m_stack) m_stack->setCurrentIndex(5);
    setNavChecked(5);

    // 把这个会话加入左侧列表（即使服务器 sessions 为空也能聊）
    slotUpdateChatEntry(sellerId, bookId, title, 0);

    // 进入会话
    enterChat(sellerId, bookId, title);
}
void MainWindow::slotUpdateChatEntry(int peerId, int bookId, const QString &bookTitle, int unreadDelta)
{
    if (!m_chatSessionList) return;

    // 先找是否已存在
    for (int i = 0; i < m_chatSessionList->count(); ++i) {
        auto *it = m_chatSessionList->item(i);
        if (!it) continue;

        if (it->data(ROLE_CHAT_PEER).toInt() == peerId &&
            it->data(ROLE_CHAT_BOOK).toInt() == bookId) {

            int unread = it->data(ROLE_CHAT_UNREAD).toInt();
            unread = qMax(0, unread + unreadDelta);
            it->setData(ROLE_CHAT_UNREAD, unread);

            QString text = QString("用户 %1\n《%2》").arg(peerId).arg(bookTitle.isEmpty() ? "-" : bookTitle);
            if (unread > 0) text += QString("\n未读：%1").arg(unread);
            it->setText(text);
            it->setForeground(unread > 0 ? QBrush(QColor("#d12")) : QBrush(Qt::black));

            // 把最近会话顶到最上面
            auto *take = m_chatSessionList->takeItem(i);
            m_chatSessionList->insertItem(0, take);
            return;
        }
    }

    // 不存在就新建一条
    auto *item = new QListWidgetItem;
    item->setData(ROLE_CHAT_PEER, peerId);
    item->setData(ROLE_CHAT_BOOK, bookId);
    item->setData(ROLE_CHAT_TITLE, bookTitle);
    item->setData(ROLE_CHAT_UNREAD, qMax(0, unreadDelta));

    QString text = QString("用户 %1\n《%2》").arg(peerId).arg(bookTitle.isEmpty() ? "-" : bookTitle);
    if (unreadDelta > 0) text += QString("\n未读：%1").arg(unreadDelta);
    item->setText(text);
    if (unreadDelta > 0) item->setForeground(QBrush(QColor("#d12")));

    m_chatSessionList->insertItem(0, item);
}



