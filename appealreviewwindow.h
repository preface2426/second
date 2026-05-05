#ifndef APPEALREVIEWWINDOW_H
#define APPEALREVIEWWINDOW_H

#include <QDialog>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include "tcpclient.h" // 复用原有管理员端的TCP客户端类

// 管理员申诉审核窗口（贴合原有管理员端UI风格）
class AppealReviewWindow : public QDialog
{
    Q_OBJECT
public:
    explicit AppealReviewWindow(TcpClient *tcpClient, QWidget *parent = nullptr);
    ~AppealReviewWindow() override;

private slots:
    // 刷新待处理申诉列表
    void onRefreshAppealsClicked();

    // 选中申诉行，加载详情
    void onAppealItemSelected(int row, int column);

    // 审核通过按钮点击事件
    void onApproveAppealClicked();

    // 审核驳回按钮点击事件
    void onRejectAppealClicked();

private:
    // 成员变量
    TcpClient *m_tcpClient;
    QTableWidget *m_tblAppeals;       // 待处理申诉列表
    QTextEdit *m_teAppealDetail;      // 申诉详情展示
    QTextEdit *m_teHandleOpinion;     // 处理意见输入
    QPushButton *m_btnRefresh;        // 刷新按钮
    QPushButton *m_btnApprove;        // 审核通过按钮
    QPushButton *m_btnReject;         // 审核驳回按钮
    QString m_currentAppealId;        // 当前选中的申诉ID

    // 初始化UI
    void initUI();

    // 加载待处理申诉列表
    void loadPendingAppeals();

    // 提交审核结果到服务端
    void submitHandleResult(int status, const QString &opinion);
};

#endif // APPEALREVIEWWINDOW_H
