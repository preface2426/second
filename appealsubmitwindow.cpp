#include "appealsubmitwindow.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>

AppealSubmitWindow::AppealSubmitWindow(int buyerId, TcpClient *tcpClient, QWidget *parent)
    : QWidget(parent), m_buyerId(buyerId), m_tcpClient(tcpClient)
{
    initUI();
}

AppealSubmitWindow::~AppealSubmitWindow()
{
}

void AppealSubmitWindow::presetFromBook(int bookId, const QString &sellerName, const QString &bookTitle)
{
    // 允许部分字段为空（例如 sellerName 暂时未知），但尽量填充
    if (m_leBookId)     m_leBookId->setText(bookId > 0 ? QString::number(bookId) : QString());
    if (m_leBookTitle)  m_leBookTitle->setText(bookTitle);
    if (m_leSellerName) m_leSellerName->setText(sellerName);

    // 给一个明确提示
    if (m_labTip) {
        m_labTip->setStyleSheet("color:#666;");
        m_labTip->setText("提示：已从你选择的订单/购物车商品自动带入卖家和书籍信息，请填写申诉理由后提交。");
    }
}

void AppealSubmitWindow::refresh()
{
    // Tab 页面一般不需要强制刷新，这里留空即可
}

void AppealSubmitWindow::initUI()
{
    m_leSellerName = new QLineEdit(this);
    m_leBookTitle  = new QLineEdit(this);
    m_leBookId     = new QLineEdit(this);
    m_teAppealReason = new QTextEdit(this);
    m_cbxCompensation = new QComboBox(this);
    m_btnSubmit = new QPushButton("提交申诉", this);
    m_btnReset  = new QPushButton("重置表单", this);

    m_labTip = new QLabel(this);
    m_labTip->setText("提示：请填写卖家、书籍信息和申诉理由。提交后等待管理员审核，结果会通过系统公告通知。");
    m_labTip->setStyleSheet("color:#666;");

    m_leSellerName->setPlaceholderText("请输入卖家用户名");
    m_leBookTitle->setPlaceholderText("请输入书籍标题");
    m_leBookId->setPlaceholderText("请输入书籍ID");
    m_teAppealReason->setPlaceholderText("请详细描述申诉理由（如书籍质量问题、残缺等）");
    m_cbxCompensation->addItems({"退货退款", "更换同规格合格商品"});

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 14, 18, 14);
    mainLayout->setSpacing(10);

    auto *title = new QLabel("申诉退款（买家）");
    title->setStyleSheet("font-size:16px; font-weight:700;");
    mainLayout->addWidget(title);
    mainLayout->addWidget(m_labTip);

    QGridLayout *formLayout = new QGridLayout();
    formLayout->setHorizontalSpacing(10);
    formLayout->setVerticalSpacing(10);

    formLayout->addWidget(new QLabel("卖家用户名："), 0, 0);
    formLayout->addWidget(m_leSellerName, 0, 1);

    formLayout->addWidget(new QLabel("书籍标题："), 1, 0);
    formLayout->addWidget(m_leBookTitle, 1, 1);

    formLayout->addWidget(new QLabel("书籍ID："), 2, 0);
    formLayout->addWidget(m_leBookId, 2, 1);

    formLayout->addWidget(new QLabel("赔偿方式："), 3, 0);
    formLayout->addWidget(m_cbxCompensation, 3, 1);

    formLayout->addWidget(new QLabel("申诉理由："), 4, 0);
    formLayout->addWidget(m_teAppealReason, 4, 1, 2, 1);

    mainLayout->addLayout(formLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnSubmit);
    btnLayout->addWidget(m_btnReset);
    mainLayout->addLayout(btnLayout);

    connect(m_btnSubmit, &QPushButton::clicked, this, &AppealSubmitWindow::onSubmitAppealClicked);
    connect(m_btnReset, &QPushButton::clicked, this, &AppealSubmitWindow::onResetFormClicked);

    this->setStyleSheet(QLatin1String(
        "QPushButton { padding: 6px 12px; margin: 5px; }"
        "QLineEdit, QTextEdit, QComboBox { padding: 4px; margin: 3px; }"
    ));
}

bool AppealSubmitWindow::validateForm()
{
    if (m_leSellerName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "表单验证失败", "请输入卖家用户名");
        return false;
    }
    if (m_leBookTitle->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "表单验证失败", "请输入书籍标题");
        return false;
    }
    if (m_leBookId->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "表单验证失败", "请输入书籍ID");
        return false;
    }
    if (m_teAppealReason->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "表单验证失败", "请输入申诉理由");
        return false;
    }
    return true;
}

void AppealSubmitWindow::onSubmitAppealClicked()
{
    if (!validateForm()) return;

    QJsonObject request;
    request["action"] = "submit_appeal";
    request["buyer_id"] = m_buyerId;
    request["seller_name"] = m_leSellerName->text().trimmed();


    request["book_title"] = m_leBookTitle->text().trimmed();
    request["book_id"] = m_leBookId->text().trimmed();
    request["appeal_reason"] = m_teAppealReason->toPlainText().trimmed();
    request["compensation_type"] = m_cbxCompensation->currentText();

    QJsonObject response = m_tcpClient->sendRequest(request);

    const bool okByStatus = response.value("status").toString() == "success";
    const bool okByCode = response.contains("code") && response.value("code").toInt() == 0;

    if (okByStatus || okByCode) {
        m_labTip->setStyleSheet("color:#1e7f55; font-weight:600;");
        QString msg = response.value("message").toString();
        if (msg.isEmpty()) msg = "申诉已提交，等待管理员审核。审核结果会通过系统公告通知。";
        m_labTip->setText("提交成功：" + msg);
        onResetFormClicked();
    } else {
        m_labTip->setStyleSheet("color:#b00020; font-weight:600;");
        m_labTip->setText("提交失败：" + response.value("message").toString("服务器未实现/无响应"));
    }
}

void AppealSubmitWindow::onResetFormClicked()
{
    m_leSellerName->clear();
    m_leBookTitle->clear();
    m_leBookId->clear();
    m_teAppealReason->clear();
    m_cbxCompensation->setCurrentIndex(0);
}
