#pragma once
#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>
#include "tcpclient.h"

class AppealReviewWindow;

class AdminWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit AdminWindow(QWidget *parent = nullptr);

private slots:
    void slotLogin();
    void slotRefresh();
    void slotDeleteSelected();
    void slotOpenAppealReview();

private:
    void createLoginUI();
    void createMainUI();
    void loadBooksToList();

    QWidget* makeBookCardWidget(const QJsonObject &book);

private:
    TcpClient *m_client = nullptr;

    // login ui
    QWidget *m_loginPage = nullptr;
    QLineEdit *m_leUser = nullptr;
    QLineEdit *m_lePwd  = nullptr;

    // main ui
    QWidget *m_mainPage = nullptr;
    QListWidget *m_list = nullptr;

    QPushButton *m_btnRefresh = nullptr;
    QPushButton *m_btnDelete = nullptr;
    QPushButton *m_btnAppeal = nullptr;

    QLabel *m_labHint = nullptr;
};
