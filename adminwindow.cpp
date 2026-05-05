#include "adminwindow.h"
#include "appealreviewwindow.h"
#include <QtWidgets>

static constexpr int ROLE_BOOK_ID   = Qt::UserRole;
static constexpr int ROLE_TITLE     = Qt::UserRole + 1;
static constexpr int ROLE_SELLER_ID = Qt::UserRole + 2;
static constexpr int ROLE_PRICE     = Qt::UserRole + 3;
static constexpr int ROLE_IS_SOLD   = Qt::UserRole + 4;

AdminWindow::AdminWindow(QWidget *parent)
    : QMainWindow(parent), m_client(new TcpClient(this))
{
    setWindowTitle("管理员端");
    resize(900, 650);
    setMinimumSize(800, 600);

    if (!m_client->connectToServer()) {
        QMessageBox::critical(this, "网络", "无法连接服务器！");
        qApp->quit();
        return;
    }

    createLoginUI();
}

void AdminWindow::createLoginUI()
{
    menuBar()->clear();

    m_loginPage = new QWidget(this);
    setCentralWidget(m_loginPage);

    auto *outer = new QVBoxLayout(m_loginPage);
    outer->setContentsMargins(60, 50, 60, 50);
    outer->setSpacing(16);

    auto *title = new QLabel("管理员登录");
    title->setStyleSheet("font-size:22px;font-weight:800;");
    outer->addWidget(title);

    auto *form = new QFormLayout;
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);

    m_leUser = new QLineEdit;
    m_lePwd  = new QLineEdit;
    m_lePwd->setEchoMode(QLineEdit::Password);

    m_leUser->setPlaceholderText("admin");
    m_lePwd->setPlaceholderText("123456");

    form->addRow("账号：", m_leUser);
    form->addRow("密码：", m_lePwd);

    outer->addLayout(form);

    auto *btnRow = new QHBoxLayout;
    auto *btnLogin = new QPushButton("登录");
    btnLogin->setObjectName("PrimaryBtn");

    auto *btnExit = new QPushButton("退出");
    btnExit->setObjectName("GhostBtn");

    btnRow->addWidget(btnLogin);
    btnRow->addWidget(btnExit);
    btnRow->addStretch();
    outer->addLayout(btnRow);

    auto *tip = new QLabel("提示：默认账号 admin，密码 123456（你也可以自行改成服务端校验）");
    tip->setStyleSheet("color:#666;");
    tip->setWordWrap(true);
    outer->addWidget(tip);

    connect(btnLogin, &QPushButton::clicked, this, &AdminWindow::slotLogin);
    connect(btnExit, &QPushButton::clicked, this, []{ qApp->quit(); });
}

void AdminWindow::slotLogin()
{
    const QString u = m_leUser->text().trimmed();
    const QString p = m_lePwd->text();

    if (u != "admin" || p != "123456") {
        QMessageBox::warning(this, "登录失败", "管理员账号或密码错误");
        return;
    }

    createMainUI();
    slotRefresh();
}

void AdminWindow::createMainUI()
{
    menuBar()->clear();

    auto *menu = menuBar()->addMenu("操作");
    menu->addAction("刷新列表", this, &AdminWindow::slotRefresh);
    menu->addAction("删除选中", this, &AdminWindow::slotDeleteSelected);
    menu->addAction("申诉审核", this, &AdminWindow::slotOpenAppealReview);

    m_mainPage = new QWidget(this);
    setCentralWidget(m_mainPage);

    auto *root = new QVBoxLayout(m_mainPage);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    auto *headRow = new QHBoxLayout;

    auto *lab = new QLabel("书籍管理（选中一项后可删除）");
    lab->setStyleSheet("font-size:18px;font-weight:800;");
    headRow->addWidget(lab);

    headRow->addStretch();

    m_btnRefresh = new QPushButton("刷新");
    m_btnRefresh->setObjectName("GhostBtn");

    m_btnDelete = new QPushButton("删除选中");
    m_btnDelete->setObjectName("DangerBtn");

    m_btnAppeal = new QPushButton("申诉审核");
    m_btnAppeal->setObjectName("PrimaryBtn");

    headRow->addWidget(m_btnRefresh);
    headRow->addWidget(m_btnDelete);
    headRow->addWidget(m_btnAppeal);

    root->addLayout(headRow);

    m_labHint = new QLabel("说明：删除会向服务器发送 delete_book 请求（需服务端支持该 action）。");
    m_labHint->setStyleSheet("color:#666;");
    m_labHint->setWordWrap(true);
    root->addWidget(m_labHint);

    m_list = new QListWidget;
    m_list->setSpacing(6);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_list, 1);

    connect(m_btnRefresh, &QPushButton::clicked, this, &AdminWindow::slotRefresh);
    connect(m_btnDelete, &QPushButton::clicked, this, &AdminWindow::slotDeleteSelected);
    connect(m_btnAppeal, &QPushButton::clicked, this, &AdminWindow::slotOpenAppealReview);
}

void AdminWindow::slotOpenAppealReview()
{
    AppealReviewWindow dlg(m_client, this);
    dlg.exec();
}

void AdminWindow::slotRefresh()
{
    loadBooksToList();
}

void AdminWindow::loadBooksToList()
{
    if (!m_list) return;

    m_list->clear();

    QJsonObject req{{"action","list_books"}};
    QJsonObject resp = m_client->sendRequest(req);

    QJsonArray books = resp.value("books").toArray();
    if (books.isEmpty()) {
        auto *it = new QListWidgetItem("暂无书籍");
        it->setFlags(Qt::NoItemFlags);
        m_list->addItem(it);
        return;
    }

    for (auto v : books) {
        QJsonObject b = v.toObject();

        int bookId   = b.value("id").toInt();
        QString title = b.value("title").toString();
        double price  = b.value("price").toDouble();
        int sellerId  = b.value("seller_id").toInt();
        int isSold    = b.value("is_sold").toInt();

        auto *item = new QListWidgetItem;
        item->setSizeHint(QSize(0, 105));
        item->setData(ROLE_BOOK_ID, bookId);
        item->setData(ROLE_TITLE, title);
        item->setData(ROLE_PRICE, price);
        item->setData(ROLE_SELLER_ID, sellerId);
        item->setData(ROLE_IS_SOLD, isSold);

        m_list->addItem(item);
        m_list->setItemWidget(item, makeBookCardWidget(b));
    }
}

QWidget* AdminWindow::makeBookCardWidget(const QJsonObject &book)
{
    int bookId = book.value("id").toInt();
    QString title = book.value("title").toString();
    double price = book.value("price").toDouble();
    int sellerId = book.value("seller_id").toInt();
    bool sold = book.value("is_sold").toInt() != 0;

    auto *card = new QFrame;
    card->setObjectName("BookCard");
    auto *hl = new QHBoxLayout(card);
    hl->setContentsMargins(16, 12, 16, 12);
    hl->setSpacing(12);

    auto *vl = new QVBoxLayout;
    vl->setSpacing(6);

    auto *labTitle = new QLabel(QString("《%1》").arg(title));
    labTitle->setStyleSheet("font-size:16px;font-weight:800;");
    labTitle->setWordWrap(true);

    auto *labInfo = new QLabel(QString("ID：%1   卖家ID：%2   价格：￥%3")
                               .arg(bookId)
                               .arg(sellerId)
                               .arg(price, 0, 'f', 2));
    labInfo->setStyleSheet("color:#555;");

    vl->addWidget(labTitle);
    vl->addWidget(labInfo);

    hl->addLayout(vl, 1);

    auto *badge = new QLabel(sold ? "已售出" : "在售");
    badge->setObjectName(sold ? "BadgeSold" : "BadgeOnSale");
    hl->addWidget(badge, 0, Qt::AlignTop);

    return card;
}

void AdminWindow::slotDeleteSelected()
{
    if (!m_list) return;

    auto *it = m_list->currentItem();
    if (!it) {
        QMessageBox::information(this, "删除", "请先选中要删除的书籍");
        return;
    }

    int bookId = it->data(ROLE_BOOK_ID).toInt();
    QString title = it->data(ROLE_TITLE).toString();

    auto ret = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除：\n《%1》 (id=%2) ？").arg(title).arg(bookId),
        QMessageBox::Yes | QMessageBox::No
    );
    if (ret != QMessageBox::Yes) return;

    QJsonObject req{{"action", "delete_book"}, {"book_id", bookId}};
    QJsonObject resp = m_client->sendRequest(req);

    if (resp.value("status").toString() == "success") {
        QString sellerAlipay = resp.value("seller_alipay").toString();
        QMessageBox::information(this, "操作成功",
                                 QString("书籍已删除！\n卖家支付宝沙盒账户：%1")
                                     .arg(sellerAlipay.isEmpty() ? "未知" : sellerAlipay));
        slotRefresh();
    } else {
        QMessageBox::warning(this, "操作失败",
                             "删除失败：可能服务器未实现 delete_book 或该书已不存在");
    }
}
