#pragma once
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

class AnimatedButton : public QPushButton
{
    Q_OBJECT
public:
    explicit AnimatedButton(const QString &text, QWidget *parent = nullptr);

protected:
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void animateTo(int yOffset, qreal shadowBlur, const QPoint &shadowOffset, int duration);

    QPoint m_basePos;
    bool m_hasBasePos = false;

    QGraphicsDropShadowEffect *m_shadow = nullptr;
    QPropertyAnimation *m_posAnim = nullptr;
    QPropertyAnimation *m_blurAnim = nullptr;
    QPropertyAnimation *m_offsetAnim = nullptr;
};
