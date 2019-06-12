#include "qitemcb.h"
#include "ui_qitemcb.h"
#include <QFileDialog>
#include <QPixmap>

QItemCB::QItemCB(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::QItemCB)
{
    ui->setupUi(this);
    this->m_strImageFile = "white";
}

QItemCB::~QItemCB()
{
    delete ui;
}

void QItemCB::on_chkImage_stateChanged(int nState)
{
    if (nState == Qt::Checked) {
        QString strFile = QFileDialog::getOpenFileName(
                              this,
                              "Select Background Image",
                              "./",
                              "Image (*.png *.jpg)");
        if (strFile.isEmpty()) return;

        this->m_strImageFile = strFile;

        QPixmap pixImage(strFile);
        QSize szLabel = this->ui->lblImage->size();
        QPixmap pixLabel = pixImage.scaled(szLabel, Qt::KeepAspectRatio);
        this->ui->lblImage->setPixmap(pixLabel);
        this->ui->lblImage->setStyleSheet("background-color: rgba(255, 255, 255, 0);");
        emit this->valueChanged(this->m_strImageFile);
    } else if (nState == Qt::Unchecked) {
        this->m_strImageFile = "white";
        this->ui->lblImage->clear();
        this->ui->lblImage->setStyleSheet("background-color: rgba(255, 255, 255, 255);");
        emit this->valueChanged(this->m_strImageFile);
    }
}

QString QItemCB::value()
{
    return this->m_strImageFile;
}

void QItemCB::setValue(QString strImage)
{
    this->ui->chkImage->disconnect(SIGNAL(stateChanged(int)));

    this->m_strImageFile = strImage;
    if (this->m_strImageFile != "white") {
        QPixmap pixImage(this->m_strImageFile);
        QSize szLabel = this->ui->lblImage->size();
        QPixmap pixLabel = pixImage.scaled(szLabel, Qt::KeepAspectRatio);
        this->ui->lblImage->setPixmap(pixLabel);
        this->ui->lblImage->setStyleSheet("background-color: rgba(255, 255, 255, 0);");
        this->ui->chkImage->setCheckState(Qt::Checked);
    } else {
        this->ui->lblImage->clear();
        this->ui->lblImage->setStyleSheet("background-color: rgba(255, 255, 255, 255);");
        this->ui->chkImage->setCheckState(Qt::Unchecked);
    }

    this->connect(this->ui->chkImage, SIGNAL(stateChanged(int)), this, SLOT(on_chkImage_stateChanged(int)));
}
