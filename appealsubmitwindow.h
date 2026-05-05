#ifndef APPEALSUBMITWINDOW_H
#define APPEALSUBMITWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include "tcpclient.h"

// 买家申诉提交页面（从 QDialog 改成 QWidget，用作 Tab）
class AppealSubmitWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AppealSubmitWindow(int buyerId, TcpClient *tcpClient, QWidget *parent = nullptr);
    ~AppealSubmitWindow() override;

    // ✅ 由 MainWindow 在切换到「申诉」Tab 前调用：自动填充
    void presetFromBook(int bookId, const QString &sellerName, const QString &bookTitle);

public slots:
    void refresh();

private slots:
    void onSubmitAppealClicked();
    void onResetFormClicked();

private:
    int m_buyerId;
    TcpClient *m_tcpClient;

    // UI组件
    QLineEdit *m_leSellerName;
    QLineEdit *m_leBookTitle;
    QLineEdit *m_leBookId;
    QTextEdit *m_teAppealReason;
    QComboBox *m_cbxCompensation;
    QPushButton *m_btnSubmit;
    QPushButton *m_btnReset;
    QLabel *m_labTip;

    void initUI();
    bool validateForm();
};

#endif // APPEALSUBMITWINDOW_H
