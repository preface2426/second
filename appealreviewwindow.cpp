#include "appealreviewwindow.h"
#include <QMessageBox>
#include <QJsonArray>
#include <QHeaderView>
#include <QLabel>

AppealReviewWindow::AppealReviewWindow(TcpClient *tcpClient, QWidget *parent)
    : QDialog(parent), m_tcpClient(tcpClient)
{
    this->setWindowTitle("二手书申诉审核");
    this->setFixedSize(800, 500);
    initUI();
    loadPendingAppeals();
}

AppealReviewWindow::~AppealReviewWindow()
{
}

void AppealReviewWindow::initUI()
{
    m_tblAppeals = new QTableWidget(this);
    m_teAppealDetail = new QTextEdit(this);
    m_teHandleOpinion = new QTextEdit(this);
    m_btnRefresh = new QPushButton("刷新列表", this);
    m_btnApprove = new QPushButton("审核通过", this);
    m_btnReject = new QPushButton("审核驳回", this);

    m_tblAppeals->setColumnCount(5);
    m_tblAppeals->setHorizontalHeaderLabels({"申诉ID", "卖家用户名", "书籍标题", "申诉时间", "状态"});
    m_tblAppeals->horizontalHeader()->setStretchLastSection(true);
    m_tblAppeals->setEditTriggers(QTableWidget::NoEditTriggers);

    m_teAppealDetail->setReadOnly(true);
    m_teHandleOpinion->setPlaceholderText("请输入处理意见（审核通过请说明后续要求，审核驳回请说明理由）");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout();
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QHBoxLayout *btnLayout = new QHBoxLayout();

    leftLayout->addWidget(new QLabel("待处理申诉列表："));
    leftLayout->addWidget(m_tblAppeals);

    rightLayout->addWidget(new QLabel("申诉详情："));
    rightLayout->addWidget(m_teAppealDetail);
    rightLayout->addWidget(new QLabel("处理意见："));
    rightLayout->addWidget(m_teHandleOpinion);

    topLayout->addLayout(leftLayout, 1);
    topLayout->addLayout(rightLayout, 1);

    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRefresh);
    btnLayout->addWidget(m_btnApprove);
    btnLayout->addWidget(m_btnReject);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(btnLayout);

    connect(m_btnRefresh, &QPushButton::clicked, this, &AppealReviewWindow::onRefreshAppealsClicked);
    connect(m_tblAppeals, &QTableWidget::cellClicked, this, &AppealReviewWindow::onAppealItemSelected);
    connect(m_btnApprove, &QPushButton::clicked, this, &AppealReviewWindow::onApproveAppealClicked);
    connect(m_btnReject, &QPushButton::clicked, this, &AppealReviewWindow::onRejectAppealClicked);

    this->setStyleSheet(QLatin1String("QPushButton { padding: 8px 16px; margin: 5px; }"
                                      "QTableWidget { gridline-color: #cccccc; }"
                                      "QTextEdit { padding: 4px; margin: 3px; }"));
}

void AppealReviewWindow::loadPendingAppeals()
{
    QJsonObject request;
    request["action"] = "get_pending_appeals";

    QJsonObject response = m_tcpClient->sendRequest(request);

    const bool ok = (response.contains("code") && response["code"].toInt() == 0)
                    || (response.value("status").toString() == "success");
    if (!ok) {
        QMessageBox::critical(this, "加载失败", "获取待处理申诉列表失败");
        return;
    }

    m_tblAppeals->setRowCount(0);

    QJsonArray appealArray = response["data"].toArray();
    for (int i = 0; i < appealArray.size(); ++i) {
        QJsonObject appeal = appealArray[i].toObject();
        m_tblAppeals->insertRow(i);
        m_tblAppeals->setItem(i, 0, new QTableWidgetItem(appeal["appeal_id"].toString()));
        m_tblAppeals->setItem(i, 1, new QTableWidgetItem(appeal["seller_name"].toString()));
        m_tblAppeals->setItem(i, 2, new QTableWidgetItem(appeal["book_title"].toString()));
        m_tblAppeals->setItem(i, 3, new QTableWidgetItem(appeal["appeal_time"].toString()));
        m_tblAppeals->setItem(i, 4, new QTableWidgetItem("待审核"));
    }
}

void AppealReviewWindow::onRefreshAppealsClicked()
{
    loadPendingAppeals();
    m_teAppealDetail->clear();
    m_teHandleOpinion->clear();
    m_currentAppealId.clear();
}

void AppealReviewWindow::onAppealItemSelected(int row, int column)
{
    Q_UNUSED(column);

    m_currentAppealId = m_tblAppeals->item(row, 0)->text();

    QString detailText;
    detailText += "申诉ID：" + m_currentAppealId + "\n";
    detailText += "卖家用户名：" + m_tblAppeals->item(row, 1)->text() + "\n";
    detailText += "书籍标题：" + m_tblAppeals->item(row, 2)->text() + "\n";
    detailText += "申诉时间：" + m_tblAppeals->item(row, 3)->text() + "\n";

    QJsonObject request;
    request["action"] = "get_appeal_detail";
    request["appeal_id"] = m_currentAppealId;

    QJsonObject response = m_tcpClient->sendRequest(request);

    const bool ok = (response.contains("code") && response["code"].toInt() == 0)
                    || (response.value("status").toString() == "success");
    if (ok) {
        QJsonObject appealDetail = response["appeal_detail"].toObject();
        detailText += "书籍ID：" + appealDetail["book_id"].toString() + "\n";
        detailText += "赔偿要求：" + appealDetail["compensation_type"].toString() + "\n";
        detailText += "申诉理由：\n" + appealDetail["appeal_reason"].toString() + "\n";
    }

    m_teAppealDetail->setText(detailText);
}

void AppealReviewWindow::submitHandleResult(int status, const QString &opinion)
{
    if (m_currentAppealId.isEmpty()) {
        QMessageBox::warning(this, "操作失败", "请先选中一条申诉记录");
        return;
    }
    if (opinion.isEmpty()) {
        QMessageBox::warning(this, "操作失败", "请输入处理意见");
        return;
    }

    QJsonObject request;
    request["action"] = "handle_appeal";
    request["appeal_id"] = m_currentAppealId;
    request["status"] = status;
    request["handle_opinion"] = opinion;

    QJsonObject response = m_tcpClient->sendRequest(request);

    const bool ok = (response.contains("code") && response["code"].toInt() == 0)
                    || (response.value("status").toString() == "success");
    if (ok) {
        QMessageBox::information(this, "操作成功", "申诉处理结果已提交");
        loadPendingAppeals();
    } else {
        QMessageBox::critical(this, "操作失败", "申诉处理结果提交失败");
    }
}

void AppealReviewWindow::onApproveAppealClicked()
{
    submitHandleResult(1, m_teHandleOpinion->toPlainText());
}

void AppealReviewWindow::onRejectAppealClicked()
{
    submitHandleResult(2, m_teHandleOpinion->toPlainText());
}
