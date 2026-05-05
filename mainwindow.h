#pragma once

#include <QMainWindow>
#include <QHash>
#include <QMap>
#include <QPixmap>
#include <QJsonObject>
#include <QTimer>
#include <QPushButton>

#include "tcpclient.h"
class BuyHistoryWindow;
class SellerMsgWindow;
class BuyHistoryWindow;
class AppealSubmitWindow;

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QListWidget;
class QLineEdit;
class QLabel;
class QListWidgetItem;
class QTextBrowser;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    // auth
    void slotLogin();
    void slotRegister();

    // pages/actions
    void slotBrowse();
    void slotSearch();
    void slotMyBooks();
    void slotPostBook();

    // cart
    void slotAddToCart(QListWidgetItem *item);
    void slotRemoveFromCart();
    void slotClearCart();
    void slotCheckout();

    // chat
    void slotOpenChat();
    void slotChatSend();
    void slotChatTick();

    // ✅ 新增：卖家通知/历史购买/申诉
    void slotOpenSellerMsg();
    void slotOpenBuyHistory();
    void slotOpenAppealSubmit();

private:
    void createLoginUI();
    void createMainUI();

    QWidget* makeBookRowWidget(const QJsonObject &book, int &outBookId);
    QPixmap fetchCoverPixmap(const QString &imagePath);

    void refreshCartTotalLabel();
    void removeCartItemFromUI(int bookId);

    void setupHoverPreview();
    void showHoverPreview(QListWidget *list, QListWidgetItem *item, const QPoint &globalPos);
    void hideHoverPreview();

    void enterChat(int peerId, int bookId, const QString &bookTitle);
    void refreshChatSessionList();
    void appendChatMessage(const QJsonObject &msg);

    void setNavChecked(int pageIndex);
    void slotChatWithSelectedBook();
    void slotUpdateChatEntry(int peerId, int bookId, const QString &bookTitle, int unreadDelta = 0);

private:
    TcpClient *m_client = nullptr;
    int m_userId = -1;

    QHash<QString, QPixmap> m_coverCache;
    QStackedWidget *m_stack = nullptr;

    QListWidget *m_listAll = nullptr;
    QListWidget *m_listSearch = nullptr;
    QListWidget *m_listMine = nullptr;

    // login
    QLineEdit *m_leUser = nullptr;
    QLineEdit *m_lePwd  = nullptr;
    QLineEdit *m_leAlipay = nullptr;

    // search
    QLineEdit *m_leKeyword = nullptr;

    // post
    QLineEdit *m_leTitle = nullptr;
    QLineEdit *m_leCategory = nullptr;
    QLineEdit *m_lePrice = nullptr;
    QLabel *m_labPic = nullptr;
    QString m_localImagePath;

    // cart
    struct CartItem {
        int bookId = -1;
        int sellerId = -1;
        QString sellerName;
        QString title;
        double price = 0.0;
        int isSold = 0;
        QString imagePath;
    };
    QMap<int, CartItem> m_cart;
    double m_cartTotal = 0.0;

    QListWidget *m_listCart = nullptr;
    QLabel *m_labTotal = nullptr;

    // hover preview
    QTimer *m_hoverTimer = nullptr;
    QWidget *m_hoverPopup = nullptr;
    QLabel *m_hoverImage = nullptr;
    QLabel *m_hoverTitle = nullptr;
    QLabel *m_hoverPrice = nullptr;
    QLabel *m_hoverStatus = nullptr;
    QLabel *m_hoverId = nullptr;

    QListWidget *m_hoverList = nullptr;
    QListWidgetItem *m_hoverItem = nullptr;
    QPoint m_hoverGlobalPos;

    // chat
    QListWidget *m_chatSessionList = nullptr;
    QLabel *m_chatInfo = nullptr;
    QTextBrowser *m_chatView = nullptr;
    QLineEdit *m_chatInput = nullptr;
    QPushButton *m_chatSendBtn = nullptr;
    QTimer *m_chatTimer = nullptr;

    int m_chatPeerId = -1;
    int m_chatBookId = -1;
    QString m_chatBookTitle;
    int m_chatLastMsgId = 0;

    // bottom nav buttons
    QPushButton *m_navBrowseBtn = nullptr;
    QPushButton *m_navSearchBtn = nullptr;
    QPushButton *m_navPostBtn   = nullptr;
    QPushButton *m_navCartBtn   = nullptr;
    QPushButton *m_navChatBtn   = nullptr;
    QPushButton *m_navMineBtn   = nullptr;

    // ✅ 新增3个按钮
    QPushButton *m_navSellerMsgBtn = nullptr;
    QPushButton *m_navHistoryBtn   = nullptr;
    QPushButton *m_navAppealBtn    = nullptr;

    // ✅ 3个页面
    SellerMsgWindow *m_sellerMsgPage = nullptr;   // 卖家通知
    BuyHistoryWindow *m_buyHistoryPage = nullptr; // 历史订单
    AppealSubmitWindow *m_appealPage = nullptr;   // 申诉

};
