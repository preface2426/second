#include "animatedbutton.h"
#include <QMouseEvent>

AnimatedButton::AnimatedButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    setCursor(Qt::PointingHandCursor);

    // 阴影
    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(18);
    m_shadow->setOffset(0, 6);
    m_shadow->setColor(QColor(20, 20, 30, 70));
    setGraphicsEffect(m_shadow);

    // 动画：位置、阴影模糊、阴影偏移
    m_posAnim = new QPropertyAnimation(this, "pos", this);
    m_posAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_blurAnim = new QPropertyAnimation(m_shadow, "blurRadius", this);
    m_blurAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_offsetAnim = new QPropertyAnimation(m_shadow, "offset", this);
    m_offsetAnim->setEasingCurve(QEasingCurve::OutCubic);
}

void AnimatedButton::animateTo(int yOffset, qreal shadowBlur, const QPoint &shadowOffset, int duration)
{
    if (!m_hasBasePos) {
        m_basePos = pos();
        m_hasBasePos = true;
    }

    m_posAnim->stop();
    m_blurAnim->stop();
    m_offsetAnim->stop();

    m_posAnim->setDuration(duration);
    m_posAnim->setStartValue(pos());
    m_posAnim->setEndValue(QPoint(m_basePos.x(), m_basePos.y() + yOffset));
    m_posAnim->start();

    m_blurAnim->setDuration(duration);
    m_blurAnim->setStartValue(m_shadow->blurRadius());
    m_blurAnim->setEndValue(shadowBlur);
    m_blurAnim->start();

    m_offsetAnim->setDuration(duration);
    m_offsetAnim->setStartValue(m_shadow->offset());
    m_offsetAnim->setEndValue(shadowOffset);
    m_offsetAnim->start();
}
void AnimatedButton::enterEvent(QEvent *e)
{
    QPushButton::enterEvent(e);
    animateTo(-2, 26, QPoint(0, 10), 150); // 轻微上浮 + 阴影更明显
}

void AnimatedButton::leaveEvent(QEvent *e)
{
    QPushButton::leaveEvent(e);
    animateTo(0, 18, QPoint(0, 6), 180); // 回到原位
}

void AnimatedButton::mousePressEvent(QMouseEvent *e)
{
    QPushButton::mousePressEvent(e);
    animateTo(1, 12, QPoint(0, 3), 90); // 按下：下沉一点，阴影变小
}

void AnimatedButton::mouseReleaseEvent(QMouseEvent *e)
{
    QPushButton::mouseReleaseEvent(e);
    // 松开后，如果鼠标仍在按钮上，回到 hover 状态
    if (underMouse()) animateTo(-2, 26, QPoint(0, 10), 120);
    else animateTo(0, 18, QPoint(0, 6), 160);
}
