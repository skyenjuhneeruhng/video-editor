#ifndef QITEMCB_H
#define QITEMCB_H

#include <QFrame>

namespace Ui {
class QItemCB;
}

class QItemCB : public QFrame
{
    Q_OBJECT

signals:
    void valueChanged(QString strImage);

public:
    explicit QItemCB(QWidget *parent = nullptr);
    ~QItemCB();

private slots:
    void on_chkImage_stateChanged(int nState);

private:
    Ui::QItemCB *ui;
    QString m_strImageFile;

public:
    QString value();
    void setValue(QString strImage);
};

#endif // QITEMCB_H
