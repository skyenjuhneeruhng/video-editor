#include "qlogo.h"
#include <QPainter>
#include "common.h"

QLogo::QLogo(QWidget *parent) : QLabel(parent)
{
    this->m_nMouse = N_LOGO_MOUSE_NONE;
}

void QLogo::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QSize szArea = this->size();

    // draw border lines
    QPen penBorder(Qt::black, 1, Qt::DotLine);
    painter.setPen(penBorder);

    QPolygon polyBorder;
    polyBorder << QPoint(2, 2) << QPoint(szArea.width() - 2, 2)
               << QPoint(szArea.width() - 2, szArea.height() - 2) << QPoint(2, szArea.height() - 2);

    painter.drawPolygon(polyBorder);

    // draw corner
    QPen penCorner(Qt::black, 6, Qt::DotLine);
    painter.setPen(penCorner);
    painter.drawPoint(0, 0);
    painter.drawPoint(szArea.width(), 0);
    painter.drawPoint(szArea.width(), szArea.height());
    painter.drawPoint(0, szArea.height());

    // draw pixmap
    const QPixmap *pPix = this->pixmap();

    if (pPix != nullptr) {
        painter.drawImage(QRect(0, 0, szArea.width(), szArea.height()), pPix->toImage());
    }
}

void QLogo::enterEvent(QEvent *)
{
    this->m_nMouse = N_LOGO_MOUSE_INSIDE;
}

void QLogo::leaveEvent(QEvent *)
{
    this->m_nMouse = N_LOGO_MOUSE_NONE;
}

void QLogo::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    int nX = pos.x();
    int nY = pos.y();
    int nWidth = this->width();
    int nHeight = this->height();

    if (nX <= N_LOGO_BORDER_WIDTH) {
        if (nY <= N_LOGO_BORDER_WIDTH) {
            if (nX < nY) {
                this->m_nMouse = N_LOGO_MOUSE_LEFT;
                this->setCursor(Qt::SizeHorCursor);
            } else {
                this->m_nMouse = N_LOGO_MOUSE_TOP;
                this->setCursor(Qt::SizeVerCursor);
            }
        } else if (nY >= (nHeight - N_LOGO_BORDER_WIDTH)) {
            if (nHeight - nY > nX) {
                this->m_nMouse = N_LOGO_MOUSE_LEFT;
                this->setCursor(Qt::SizeHorCursor);
            } else {
                this->m_nMouse = N_LOGO_MOUSE_TOP;
                this->setCursor(Qt::SizeVerCursor);
            }
        } else {
            this->m_nMouse = N_LOGO_MOUSE_LEFT;
            this->setCursor(Qt::SizeHorCursor);
        }
    } else if (nX >= (nWidth - N_LOGO_BORDER_WIDTH)) {
        if (nY <= N_LOGO_BORDER_WIDTH) {
            if (nY < nWidth - nX) {
                this->m_nMouse = N_LOGO_MOUSE_TOP;
                this->setCursor(Qt::SizeVerCursor);
            } else {
                this->m_nMouse = N_LOGO_MOUSE_RIGHT;
                this->setCursor(Qt::SizeHorCursor);
            }
        } else if (nY >= (nHeight - N_LOGO_BORDER_WIDTH)) {
            if (nHeight - nY < nWidth - nX) {
                this->m_nMouse = N_LOGO_MOUSE_BOTTOM;
                this->setCursor(Qt::SizeVerCursor);
            } else {
                this->m_nMouse = N_LOGO_MOUSE_RIGHT;
                this->setCursor(Qt::SizeHorCursor);
            }
        } else {
            this->m_nMouse = N_LOGO_MOUSE_RIGHT;
            this->setCursor(Qt::SizeHorCursor);
        }
    } else {
        if (nY <= N_LOGO_BORDER_WIDTH) {
            this->m_nMouse = N_LOGO_MOUSE_TOP;
            this->setCursor(Qt::SizeVerCursor);
        } else if (nY >= (nHeight - N_LOGO_BORDER_WIDTH)) {
            this->m_nMouse = N_LOGO_MOUSE_BOTTOM;
            this->setCursor(Qt::SizeVerCursor);
        } else {
            this->m_nMouse = N_LOGO_MOUSE_INSIDE;
            this->setCursor(Qt::SizeAllCursor);
        }
    }

    if (event->buttons() & Qt::LeftButton) {
        emit this->logo_mouseMove();
    }
}

int QLogo::mouse()
{
    return this->m_nMouse;
}
