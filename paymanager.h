#ifndef PAYMANAGER_H
#define PAYMANAGER_H

#include <QObject>
#include <QString>

class PayManager : public QObject {
    Q_OBJECT
public:
    explicit PayManager(QObject *parent = nullptr);
    void createPayment(QString orderId, double price, QString title);

signals:
    void urlReady(QString url);
    void errorOccurred(QString message);
};

#endif
