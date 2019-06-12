#ifndef QLOGO_H
#define QLOGO_H

#include <QLabel>
#include <QMouseEvent>
#include <QPaintDevice>
#include <QEvent>

class QLogo : public QLabel
{
    Q_OBJECT
public:
    explicit QLogo(QWidget *parent = nullptr);

signals:
    void logo_mouseMove();

public slots:

protected:
    void paintEvent(QPaintEvent *event);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void mouseMoveEvent(QMouseEvent *event);

public:
    int m_nMouse;

public:
    int mouse();
};

#endif // QLOGO_H
