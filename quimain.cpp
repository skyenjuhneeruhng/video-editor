#include "quimain.h"
#include "ui_quimain.h"

#include "common.h"
#include "qthreadplay.h"
#include "qthreadvd.h"
#include "qthreadffmpeg.h"
#include "qcv.hpp"
#include "qitemcb.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QTimeEdit>
#include <QTextEdit>
#include <QPainter>
#include <QFontDatabase>

QThreadPlay *g_pThreadPlay = nullptr;
QThreadFFMpeg *g_pThreadFFMpeg = nullptr;
QThreadVD *g_pThreadVD = nullptr;

QUIMain::QUIMain(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QUIMain)
{
    ui->setupUi(this);
    this->init();
}

QUIMain::~QUIMain()
{
    delete ui;
}

void QUIMain::init()
{
    // init splitter
    QList<int> sizes;
    sizes << 200 << this->width() - 600 << 400;
    this->ui->splitter->setSizes(sizes);

    // init batch table
    this->ui->tblBatch->setColumnCount(3);
    QStringList strlstBatchColumns;
    strlstBatchColumns << "" << "Preview" << "File";
    this->ui->tblBatch->setHorizontalHeaderLabels(strlstBatchColumns);

    QHeaderView *pHeaderBatch = this->ui->tblBatch->horizontalHeader();
    pHeaderBatch->setMinimumSectionSize(1);

    this->ui->tblBatch->setIconSize(QSize(N_BATCH_IMAGE_WIDTH, N_BATCH_IMAGE_HEIGHT));

    this->ui->tblBatch->setColumnWidth(0, 20);
    this->ui->tblBatch->setColumnWidth(1, N_BATCH_IMAGE_WIDTH);

    pHeaderBatch->setSectionResizeMode(0, QHeaderView::Fixed);
    pHeaderBatch->setSectionResizeMode(1, QHeaderView::Fixed);
    pHeaderBatch->setSectionResizeMode(2, QHeaderView::Stretch);

    // batch table column checkbox
    this->m_pchkBatch = new QCheckBox(pHeaderBatch);
    this->m_pchkBatch->move(3, 5);
    this->connect(this->m_pchkBatch, SIGNAL(stateChanged(int)), this,
                  SLOT(us_chk_stateChanged_Batch(int)));

    // init caption table
    this->ui->tblCaption->setColumnCount(5);
    QStringList strlstCaptionColumns;
    strlstCaptionColumns << "" << "Start" << "End" << "Caption" << "Background";
    this->ui->tblCaption->setHorizontalHeaderLabels(strlstCaptionColumns);

    QHeaderView *pHeaderCaption = this->ui->tblCaption->horizontalHeader();
    pHeaderCaption->setMinimumSectionSize(1);

    this->ui->tblCaption->setSelectionMode(QAbstractItemView::SingleSelection);

    this->ui->tblCaption->setColumnWidth(0, 20);
    this->ui->tblCaption->setColumnWidth(1, 100);
    this->ui->tblCaption->setColumnWidth(2, 100);
    this->ui->tblCaption->setColumnWidth(4, 150);

    pHeaderCaption->setSectionResizeMode(0, QHeaderView::Fixed);
    pHeaderCaption->setSectionResizeMode(1, QHeaderView::Fixed);
    pHeaderCaption->setSectionResizeMode(2, QHeaderView::Fixed);
    pHeaderCaption->setSectionResizeMode(3, QHeaderView::Stretch);
    pHeaderCaption->setSectionResizeMode(4, QHeaderView::Fixed);

    // caption table column checkbox
    this->m_pchkCaption = new QCheckBox(pHeaderCaption);
    this->m_pchkCaption->move(3, 5);
    this->connect(this->m_pchkCaption, SIGNAL(stateChanged(int)), this,
                  SLOT(us_chk_stateChanged_Caption(int)));

    // init intro & outro video
    this->initVideo(&this->m_videoIntro);
    this->initVideo(&this->m_videoOutro);

    // log image on preview frame
    this->m_pLogoPreview = new QLogo(this->ui->frmVideo);
    this->m_pLogoPreview->setMouseTracking(true);
    this->m_pLogoPreview->setAttribute(Qt::WA_TranslucentBackground);
    this->m_pLogoPreview->hide();

    this->connect(this->m_pLogoPreview, SIGNAL(logo_mouseMove()), this, SLOT(us_logo_mouseMove()));

    // init variable
    this->m_bIsLogoMoving = false;
    this->m_bIsPlaying = false;
    this->m_bIsRendering = false;
    this->m_timeTotal.setHMS(0, 0, 0, 0);
    this->m_timeMid.setHMS(0, 0, 0, 0);
    this->m_timeCur.setHMS(0, 0, 0, 0);
    this->m_nLogoX = N_LOGO_DEFAULT_POS_LEFT;
    this->m_nLogoY = N_LOGO_DEFAULT_POS_TOP;
    this->m_nLogoWidth = 0;
    this->m_nLogoHeight = 0;
    this->m_nLogoMouse = N_LOGO_MOUSE_NONE;
    this->m_fLogoRatio = 1;
    this->m_nCurRendering = -1;
    this->m_fTotalProgStep = 0;
    this->m_strCurFPS = "30";
    this->m_strCurScale = "3840:2160";
    this->m_strFolderTmp = "./tmp/";
    this->m_strFolderOut = "./out/";
    this->m_ThreadPriority = QThread::HighestPriority;
    this->m_fontCaption.setFamily(STR_CAPTION_FONT_FAMILY);
    this->m_fontCaption.setPointSize(N_CAPTION_FONT_SIZE);
    this->m_fontCaption.setWeight(N_CAPTION_FONT_WEIGHT);

    QFont fontCaption = this->m_fontCaption;
    fontCaption.setPointSize(16);
    this->ui->lblCaptionFont->setFont(fontCaption);

    // splitter
    this->connect(this->ui->splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(on_splitterMoved(int,
                                                                                                   int)));
}

void QUIMain::us_chk_stateChanged_Batch(int nState)
{
    qDebug("check batch - stateChaged - %d", nState);
    this->ui->tblBatch->disconnect(SIGNAL(cellChanged(int, int)));

    int nRowCount = this->ui->tblBatch->rowCount();
    if (nState == Qt::Unchecked) {
        for (int nId = 0 ; nId < nRowCount ; nId ++) {
            this->ui->tblBatch->item(nId, 0)->setCheckState(Qt::Unchecked);
        }
    } else if (nState == Qt::Checked || nState == Qt::PartiallyChecked) {
        if (nRowCount == 0) {
            this->m_pchkBatch->setCheckState(Qt::Unchecked);
            return;
        } else {
            this->m_pchkBatch->setCheckState(Qt::Checked);
        }

        for (int nId = 0 ; nId < nRowCount ; nId ++) {
            this->ui->tblBatch->item(nId, 0)->setCheckState(Qt::Checked);
        }
    }

    this->connect(this->ui->tblBatch, SIGNAL(cellChanged(int, int)), SLOT(us_tbl_cellChanged_Batch(int, int)));
}

void QUIMain::us_tbl_cellChanged_Batch(int nRow, int nCol)
{
    qDebug("table batch - cellChanged - %d, %d", nRow, nCol);
    if (nCol != 0) return;
    int nRowCount = this->ui->tblBatch->rowCount();
    int nCheckedCount = 0;
    for (int nId = 0 ; nId < nRowCount ; nId ++) {
        Qt::CheckState chkState = this->ui->tblBatch->item(nId, 0)->checkState();
        if (chkState == Qt::Checked) {
            nCheckedCount ++;
        }
    }

    this->m_pchkBatch->disconnect(SIGNAL(stateChanged(int)));

    if (nCheckedCount == nRowCount && nRowCount != 0) {
        this->m_pchkBatch->setCheckState(Qt::Checked);
    } else if (nCheckedCount == 0) {
        this->m_pchkBatch->setCheckState(Qt::Unchecked);
    } else {
        this->m_pchkBatch->setCheckState(Qt::PartiallyChecked);
    }

    this->connect(this->m_pchkBatch, SIGNAL(stateChanged(int)), this, SLOT(us_chk_stateChanged_Batch(int)));
}

void QUIMain::on_btnBatchAdd_clicked()
{
    QStringList strlstFiles = QFileDialog::getOpenFileNames(
                                  this,
                                  "Select Video Files",
                                  "./",
                                  "Video (*.avi *.mpg *.mp4 *.mov *.wmv)");

    if (strlstFiles.length() == 0) return;

    int nVideoCount = strlstFiles.length();
    this->ui->tblBatch->disconnect(SIGNAL(cellChanged(int, int)));

    // add video files to batch list
    for (int nId = 0 ; nId < nVideoCount ; nId ++) {
        Video video;
        if (this->previewVideo(strlstFiles.at(nId), &video)) {
            int nRowCount = this->ui->tblBatch->rowCount();
            this->ui->tblBatch->setRowCount(nRowCount + 1);
            qDebug("video [%s] - loaded - ", strlstFiles.at(nId).toUtf8().data());

            // checkbox
            QTableWidgetItem *pItemCheck = new QTableWidgetItem();
            pItemCheck->setCheckState(Qt::Unchecked);
            this->ui->tblBatch->setItem(nRowCount, 0, pItemCheck);

            // image
            QTableWidgetItem *pItemImage = new QTableWidgetItem();
            pItemImage->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            this->ui->tblBatch->setItem(nRowCount, 1, pItemImage);

            this->m_lstBatch.append(video);

            QPixmap pixBatchFit = video.pixPreview.scaled(QSize(N_BATCH_IMAGE_WIDTH, N_BATCH_IMAGE_HEIGHT),
                                                          Qt::KeepAspectRatio);
            pItemImage->setIcon(QIcon(pixBatchFit));

            // filename
            QFileInfo fileVideo(strlstFiles.at(nId));
            QString strFileName = fileVideo.fileName();
            QTableWidgetItem *pItemFile = new QTableWidgetItem(strFileName);
            pItemFile->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            this->ui->tblBatch->setItem(nRowCount, 2, pItemFile);

            this->ui->tblBatch->setRowHeight(nRowCount, N_BATCH_IMAGE_HEIGHT);
        } else {
            qDebug("video [%s] - failed - ", strlstFiles.at(nId).toUtf8().data());
        }
    }

    this->ui->tblBatch->selectRow(0);
    this->connect(this->ui->tblBatch, SIGNAL(cellChanged(int, int)), SLOT(us_tbl_cellChanged_Batch(int, int)));
}

void QUIMain::on_btnCaptionAdd_clicked()
{
    int nRowCount = this->ui->tblCaption->rowCount();
    this->ui->tblCaption->setRowCount(nRowCount + 1);
    this->ui->tblCaption->setRowHeight(nRowCount, 45);
    this->ui->tblCaption->disconnect(SIGNAL(cellChanged(int, int)));

    // add caption to list
    QTableWidgetItem *pItemCheck = new QTableWidgetItem();
    pItemCheck->setCheckState(Qt::Unchecked);
    this->ui->tblCaption->setItem(nRowCount, 0, pItemCheck);

    QString strStyleSheet = "selection-color: rgb(30, 30, 30);"
                            "selection-background-color: rgb(240, 240, 240);"
                            "color: rgb(255, 255, 255);";

    QTimeEdit *ptimeedtStart = new QTimeEdit(this->ui->tblCaption);
    ptimeedtStart->setStyleSheet(strStyleSheet);
    ptimeedtStart->setDisplayFormat("HH:mm:ss.zzz");
    ptimeedtStart->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    this->ui->tblCaption->setCellWidget(nRowCount, 1, ptimeedtStart);
    this->connect(ptimeedtStart, SIGNAL(timeChanged(QTime)), this, SLOT(us_updatePreview()));

    QTimeEdit *ptimeedtEnd = new QTimeEdit(this->ui->tblCaption);
    ptimeedtEnd->setStyleSheet(strStyleSheet);
    ptimeedtEnd->setDisplayFormat("HH:mm:ss.zzz");
    ptimeedtEnd->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    this->ui->tblCaption->setCellWidget(nRowCount, 2, ptimeedtEnd);
    this->connect(ptimeedtEnd, SIGNAL(timeChanged(QTime)), this, SLOT(us_updatePreview()));

    QTextEdit *ptextedtCaption = new QTextEdit(this->ui->tblCaption);
    ptextedtCaption->setStyleSheet(strStyleSheet);
    this->ui->tblCaption->setCellWidget(nRowCount, 3, ptextedtCaption);
    this->connect(ptextedtCaption, SIGNAL(textChanged()), this, SLOT(us_updatePreview()));

    QItemCB *pItemCB = new QItemCB(this->ui->tblCaption);
    this->ui->tblCaption->setCellWidget(nRowCount, 4, pItemCB);
    this->connect(pItemCB, SIGNAL(valueChanged(QString)), this, SLOT(us_updatePreview()));

    // update caption start & end time - min & max
    this->updateCaptionTime();

    this->connect(this->ui->tblCaption, SIGNAL(cellChanged(int, int)),
                  SLOT(us_tbl_cellChanged_Caption(int, int)));
}

void QUIMain::us_chk_stateChanged_Caption(int nState)
{
    qDebug("check caption - stateChaged - %d", nState);
    this->ui->tblCaption->disconnect(SIGNAL(cellChanged(int, int)));

    int nRowCount = this->ui->tblCaption->rowCount();
    if (nState == Qt::Unchecked) {
        for (int nId = 0 ; nId < nRowCount ; nId ++) {
            this->ui->tblCaption->item(nId, 0)->setCheckState(Qt::Unchecked);
        }
    } else if (nState == Qt::Checked || nState == Qt::PartiallyChecked) {
        if (nRowCount == 0) {
            this->m_pchkCaption->setCheckState(Qt::Unchecked);
            return;
        } else {
            this->m_pchkCaption->setCheckState(Qt::Checked);
        }

        for (int nId = 0 ; nId < nRowCount ; nId ++) {
            this->ui->tblCaption->item(nId, 0)->setCheckState(Qt::Checked);
        }
    }

    this->connect(this->ui->tblCaption, SIGNAL(cellChanged(int, int)),
                  SLOT(us_tbl_cellChanged_Caption(int, int)));
}

void QUIMain::us_tbl_cellChanged_Caption(int, int nCol)
{
    if (nCol != 0) return;

    int nRowCount = this->ui->tblCaption->rowCount();
    int nCheckedCount = 0;
    for (int nId = 0 ; nId < nRowCount ; nId ++) {
        Qt::CheckState chkState = this->ui->tblCaption->item(nId, 0)->checkState();
        if (chkState == Qt::Checked) {
            nCheckedCount ++;
        }
    }

    this->m_pchkCaption->disconnect(SIGNAL(stateChanged(int)));

    if (nCheckedCount == nRowCount && nRowCount != 0) {
        this->m_pchkCaption->setCheckState(Qt::Checked);
    } else if (nCheckedCount == 0) {
        this->m_pchkCaption->setCheckState(Qt::Unchecked);
    } else {
        this->m_pchkCaption->setCheckState(Qt::PartiallyChecked);
    }

    this->connect(this->m_pchkCaption, SIGNAL(stateChanged(int)), this,
                  SLOT(us_chk_stateChanged_Caption(int)));
}

bool QUIMain::previewVideo(QString strFile, Video *pVideo)
{
    cv::VideoCapture capVideo(strFile.toStdString());
    if (!capVideo.isOpened()) {
        qDebug("preview_video [%s] - video_capture - open error", strFile.toUtf8().data());
        pVideo->bAvailable = false;
        return false;
    }

    // get info
    int nFrameCount = int(capVideo.get(cv::CAP_PROP_FRAME_COUNT));
    int nFrameWidth = int(capVideo.get(cv::CAP_PROP_FRAME_WIDTH));
    int nFrameHeight = int(capVideo.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fFPS = capVideo.get(cv::CAP_PROP_FPS);
    int nFourCC = int(capVideo.get(cv::CAP_PROP_FOURCC));

    qDebug("preview_video [%s] - count - %d,  fps - %f", strFile.toUtf8().data(), nFrameCount, fFPS);

    // set random position
    int nRandomFrameNumber = rand() % (nFrameCount - 1) + 1;
    capVideo.set(cv::CAP_PROP_POS_FRAMES, nRandomFrameNumber);

    cv::Mat matFrame;
    bool bResult = capVideo.read(matFrame);
    if (!bResult) {
        qDebug("preview_video [%s] - video_capture - read error", strFile.toUtf8().data());
        pVideo->bAvailable = false;
        return false;
    }

    capVideo.release();

    QFileInfo fileInfo(strFile);

    pVideo->bAvailable = true;
    pVideo->pixPreview = QCV::cvMatToQPixmap(matFrame);
    pVideo->strFile = strFile;
    pVideo->strTitle = fileInfo.fileName();
    pVideo->nFrameCount = nFrameCount;
    pVideo->fFPS = fFPS;
    pVideo->nFourCC = nFourCC;
    pVideo->nWidth = nFrameWidth;
    pVideo->nHeight = nFrameHeight;
    pVideo->time = this->calcTime(fFPS, nFrameCount);

    return true;
}

void QUIMain::on_btnIntroLoad_clicked()
{
    QString strFile = QFileDialog::getOpenFileName(
                          this,
                          "Select Intro Video",
                          "./",
                          "Video (*.avi *.mpg *.mp4 *.mov *.wmv)");

    if (strFile.isEmpty()) return;

    // resize intro image
    QSize szPreview = this->ui->lblIntroPreview->size();

    if (this->previewVideo(strFile, &this->m_videoIntro)) {
        // set intro title
        QString strIntroTitle = this->m_videoIntro.strTitle + "  (" +
                                this->m_videoIntro.time.toString("hh:mm:ss.zzz") + ")";
        this->ui->lblIntroTitle->setText(strIntroTitle);

        // set intro preview frame
        QPixmap pixIntroFit = this->m_videoIntro.pixPreview.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblIntroPreview->setPixmap(pixIntroFit);

        this->initPreviewPanel();
        this->us_updatePreview();
    } else {
        QMessageBox::information(this, "Error", "failed to read frame.");
    }
}


void QUIMain::on_btnIntroClear_clicked()
{
    this->initVideo(&this->m_videoIntro);
    this->ui->lblIntroTitle->clear();
    this->ui->lblIntroPreview->clear();

    this->initPreviewPanel();
    this->us_updatePreview();
}

void QUIMain::on_btnLogoLoad_clicked()
{
    QString strFile = QFileDialog::getOpenFileName(
                          this,
                          "Select Logo Image",
                          "./",
                          "Image (*.png *.jpg)");
    if (strFile.isEmpty()) return;
    this->m_strLogoFile = strFile;

    QPixmap pixLogo(strFile);
    if (pixLogo.width() == 0) return;

    // logo title
    QFileInfo fileLogo(strFile);
    QString strLogoTitle = fileLogo.fileName();
    this->ui->lblLogoTitle->setText(strLogoTitle);

    // resize logo image
    QSize szPreview = this->ui->lblLogoPreview->size();
    QPixmap pixLogoFit = pixLogo.scaled(szPreview, Qt::KeepAspectRatio);
    this->ui->lblLogoPreview->setPixmap(pixLogoFit);

    // store image
    this->m_pixLogo = pixLogo;
    this->m_pLogoPreview->setPixmap(this->m_pixLogo);
    this->m_nLogoX = N_LOGO_DEFAULT_POS_LEFT;
    this->m_nLogoY = N_LOGO_DEFAULT_POS_TOP;
    this->m_nLogoWidth = 0;
    this->m_nLogoHeight = 0;
    this->m_fLogoRatio = double(pixLogo.width()) / double(pixLogo.height());

    // update ui
    this->initPreviewPanel();
    this->us_updatePreview();
}

void QUIMain::on_btnLogoClear_clicked()
{
    QPixmap pixLogo;
    this->m_pixLogo = pixLogo;
    qDebug("logo with - %d", this->m_pixLogo.width());

    this->m_nLogoX = N_LOGO_DEFAULT_POS_LEFT;
    this->m_nLogoY = N_LOGO_DEFAULT_POS_TOP;
    this->m_nLogoWidth = 0;
    this->m_nLogoHeight = 0;
    this->m_nLogoMouse = N_LOGO_MOUSE_NONE;
    this->m_fLogoRatio = 1;

    this->ui->lblLogoTitle->clear();
    this->ui->lblLogoPreview->clear();

    this->initPreviewPanel();
    this->us_updatePreview();
}

void QUIMain::on_btnOutroLoad_clicked()
{
    QString strFile = QFileDialog::getOpenFileName(
                          this,
                          "Select Outro Video",
                          "./",
                          "Video (*.avi *.mpg *.mp4 *.mov *.wmv)");
    if (strFile.isEmpty()) return;

    // resize outro image
    QSize szPreview = this->ui->lblOutroPreview->size();

    if (this->previewVideo(strFile, &this->m_videoOutro)) {
        // set outro title
        QString strOutroTitle = this->m_videoOutro.strTitle + "  (" +
                                this->m_videoOutro.time.toString("hh:mm:ss.zzz") + ")";
        this->ui->lblOutroTitle->setText(strOutroTitle);

        // set outro preview frame
        QPixmap pixOutroFit = this->m_videoOutro.pixPreview.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblOutroPreview->setPixmap(pixOutroFit);

        this->initPreviewPanel();
        this->us_updatePreview();
    } else {
        QMessageBox::information(this, "Error", "failed to read frame.");
    }
}

void QUIMain::on_btnOutroClear_clicked()
{
    this->initVideo(&this->m_videoOutro);
    this->ui->lblOutroTitle->clear();
    this->ui->lblOutroPreview->clear();

    this->initPreviewPanel();
    this->us_updatePreview();
}

void QUIMain::on_btnBatchRemove_clicked()
{
    // remove selected rows
    for (int nId = 0 ; nId < this->ui->tblBatch->rowCount() ; nId ++) {
        Qt::CheckState chkState = this->ui->tblBatch->item(nId, 0)->checkState();
        if (chkState == Qt::Checked) {
            this->ui->tblBatch->removeRow(nId);
            this->m_lstBatch.removeAt(nId);
            nId --;
        }
    }

    this->initPreviewPanel();

    // update checkbox batch
    this->us_tbl_cellChanged_Batch(0, 0);
}

void QUIMain::on_btnBatchClear_clicked()
{
    this->ui->tblBatch->clearContents();
    this->ui->tblBatch->setRowCount(0);
    this->m_pchkBatch->setCheckState(Qt::Unchecked);
    this->m_lstBatch.clear();

    this->initPreviewPanel();
}

void QUIMain::on_btnCaptionRemove_clicked()
{
    // remove selected rows
    for (int nId = 0 ; nId < this->ui->tblCaption->rowCount() ; nId ++) {
        Qt::CheckState chkState = this->ui->tblCaption->item(nId, 0)->checkState();
        if (chkState == Qt::Checked) {
            this->ui->tblCaption->removeRow(nId);
            nId --;
        }
    }

    // update checkbox batch
    this->us_tbl_cellChanged_Caption(0, 0);
}

void QUIMain::on_btnCaptionClear_clicked()
{
    this->ui->tblCaption->clearContents();
    this->ui->tblCaption->setRowCount(0);
    this->m_pchkCaption->setCheckState(Qt::Unchecked);
}

void QUIMain::us_updatePreview()
{
    // check if playing
    if (this->m_bIsPlaying) return;

    // check can show frame
    if (!this->canPlay()) {
        this->initPreviewVideo();
        return;
    }

    // current selected batch video
    int nCurVideo = this->ui->tblBatch->currentRow();

    // get preview frame width & height
    QSize szPreview = this->ui->lblVideo->size();

    this->ui->btnPlay->setEnabled(true);
    this->ui->sldTime->setEnabled(true);
    this->ui->btnRender->setEnabled(true);

    // check if current time in intro video
    if (this->m_videoIntro.bAvailable && this->m_timeCur <= this->m_videoIntro.time) {
        QPixmap pixVideoFrame;
        if (!this->grabFrame(&this->m_videoIntro, &this->m_timeCur, &pixVideoFrame)) {
            this->initPreviewVideo();
            qDebug("updatePreview - grabFrame - intro - error");
            return;
        }

        // scale to preview frame
        QPixmap pixVideoFrameFit = pixVideoFrame.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblVideo->setPixmap(pixVideoFrameFit);
        this->m_pLogoPreview->hide();
        return;
    }

    // check if current time line in outro video
    if (this->m_timeCur > this->m_timeMid) {
        QTime timeOutro = this->timeDiff(&this->m_timeCur, &this->m_timeMid);
        QPixmap pixVideoFrame;
        if (!this->grabFrame(&this->m_videoOutro, &timeOutro, &pixVideoFrame)) {
            this->initPreviewVideo();
            qDebug("updatePreview - grabFrame - outro - error");
            return;
        }

        // scale to preview frame
        QPixmap pixVideoFrameFit = pixVideoFrame.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblVideo->setPixmap(pixVideoFrameFit);
        this->m_pLogoPreview->hide();
        return;
    }

    // current time line is in batch video
    QTime timeBatch = this->timeDiff(&this->m_timeCur, &this->m_videoIntro.time);
    QPixmap pixVideoFrame;
    Video videoBatch = this->m_lstBatch.at(nCurVideo);
    if (!this->grabFrame(&videoBatch, &timeBatch, &pixVideoFrame)) {
        this->initPreviewVideo();
        qDebug("updatePreview - grabFrame - video - error");
        return;
    }

    // draw caption
    this->updateCaptionList();
    int nCaptionCount = this->m_lstCaption.length();
    for (int nId = 0 ; nId < nCaptionCount ; nId ++) {
        if (this->m_timeCur < this->m_lstCaption.at(nId).timeStart) continue;
        if (this->m_timeCur > this->m_lstCaption.at(nId).timeEnd) continue;

        // draw the first caption
        this->drawCaption(&pixVideoFrame, this->m_lstCaption.at(nId).strCaption, this->m_lstCaption.at(nId).strImage);
        break;
    }

    // scale to preview frame
    QPixmap pixVideoFrameFit = pixVideoFrame.scaled(szPreview, Qt::KeepAspectRatio);
    this->ui->lblVideo->setPixmap(pixVideoFrameFit);

    // update video frame area
    this->updateVideoAreaLogo(nCurVideo);

    // show logo
    if (this->m_pixLogo.width() != 0) {
        this->m_pLogoPreview->show();
        this->m_pLogoPreview->raise();
    }
}

void QUIMain::on_tblBatch_currentCellChanged(int, int, int, int)
{
    // initialize logo preview position
    this->m_nLogoX = N_LOGO_DEFAULT_POS_LEFT;
    this->m_nLogoY = N_LOGO_DEFAULT_POS_TOP;
    this->m_nLogoWidth = 0;
    this->m_nLogoHeight = 0;

    // initialize slider, current time position, total time position
    this->initPreviewPanel();

    // update preveiw frame
    this->us_updatePreview();
}

void QUIMain::mousePressEvent(QMouseEvent *event)
{
    this->m_nLogoMouse = this->m_pLogoPreview->mouse();
    if (event->button() == Qt::LeftButton && this->m_nLogoMouse != N_LOGO_MOUSE_NONE) {
        this->m_ptPrev = QCursor::pos();
        this->m_bIsLogoMoving = true;
    }
}

void QUIMain::mouseReleaseEvent(QMouseEvent *)
{
    this->m_bIsLogoMoving = false;
}

void QUIMain::resizeEvent(QResizeEvent *)
{
    this->updateUI();
}

void QUIMain::on_splitterMoved(int, int)
{
    this->updateUI();
}

void QUIMain::updateUI()
{
    if (this->m_videoIntro.bAvailable) {
        // update intro image
        QSize szPreview = this->ui->lblIntroPreview->size();
        QPixmap pixIntroFit = this->m_videoIntro.pixPreview.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblIntroPreview->setPixmap(pixIntroFit);
    }

    if (this->m_pixLogo.width() > 0) {
        // update logo image
        QSize szPreview = this->ui->lblLogoPreview->size();
        QPixmap pixLogoFit = this->m_pixLogo.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblLogoPreview->setPixmap(pixLogoFit);
    }

    if (this->m_videoOutro.bAvailable) {
        // update outro image
        QSize szPreview = this->ui->lblOutroPreview->size();
        QPixmap pixOutroFit = this->m_videoOutro.pixPreview.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblOutroPreview->setPixmap(pixOutroFit);
    }

    this->us_updatePreview();
}

QTime QUIMain::calcTime(double fFPS, int nFrameCount)
{
    double fSecond = double(nFrameCount) / fFPS;
    int nSecond = int(fSecond);
    int nMsec = int((fSecond - double(nSecond)) * 1000.0);
    int nMinute = nSecond / 60;
    nSecond %= 60;
    int nHour = nMinute / 60;
    nMinute %= 60;

    return QTime(nHour, nMinute, nSecond, nMsec);
}

void QUIMain::initPreviewPanel()
{
    this->m_timeCur.setHMS(0, 0, 0, 0);
    this->m_nMsecCur = 0;
    this->ui->lblTimeCur->setText("00:00:00.000");
    this->ui->sldTime->setValue(0);

    int nCurVideo = this->ui->tblBatch->currentRow();
    if (nCurVideo == -1 ||
            (!this->m_videoIntro.bAvailable && this->m_pixLogo.width() == 0 && !this->m_videoOutro.bAvailable)) {
        // if batch video list is empty or intro & logo & outro are not loaded.
        this->m_timeMid.setHMS(0, 0, 0, 0);
        this->m_timeTotal.setHMS(0, 0, 0, 0);
        this->m_nMsecTotal = 0;
        this->ui->lblTimeTotal->setText("00:00:00.000");
        this->updateCaptionTime();
        return;
    }

    int nMsecInto = abs(this->m_videoIntro.time.msecsTo(QTime(0, 0, 0, 0)));
    int nMsecOutro = abs(this->m_videoOutro.time.msecsTo(QTime(0, 0, 0, 0)));
    QTime timeVideo = this->m_lstBatch.at(nCurVideo).time;

    if (this->m_videoIntro.bAvailable && !this->m_videoOutro.bAvailable) {
        // only intro was loaded
        this->m_timeMid = timeVideo.addMSecs(nMsecInto);
        this->m_timeTotal = this->m_timeMid;
    } else if (!this->m_videoIntro.bAvailable && this->m_videoOutro.bAvailable) {
        // only outro was loaded
        this->m_timeMid  = timeVideo;
        this->m_timeTotal = timeVideo.addMSecs(nMsecOutro);
    } else if (this->m_videoIntro.bAvailable && this->m_videoOutro.bAvailable) {
        // intro & outro were loaded
        this->m_timeMid = timeVideo.addMSecs(nMsecInto);
        this->m_timeTotal = this->m_timeMid.addMSecs(nMsecOutro);
    } else {
        // only logo was loaded
        this->m_timeMid  = timeVideo;
        this->m_timeTotal = timeVideo;
    }

    this->m_nMsecTotal = abs(this->m_timeTotal.msecsTo(QTime(0, 0, 0, 0)));
    this->ui->lblTimeTotal->setText(this->m_timeTotal.toString("hh:mm:ss.zzz"));
    this->updateCaptionTime();
}

void QUIMain::updateCaptionTime()
{
    int nCaptionCount = this->ui->tblCaption->rowCount();
    for (int nId = 0 ; nId < nCaptionCount ; nId ++) {
        // start time
        QTimeEdit *ptimeedtStart = static_cast<QTimeEdit *>(this->ui->tblCaption->cellWidget(nId, 1));
        if (ptimeedtStart == nullptr) continue;
        ptimeedtStart->setMinimumTime(this->m_videoIntro.time);
        ptimeedtStart->setMaximumTime(this->m_timeMid);

        // end time
        QTimeEdit *ptimeedtEnd = static_cast<QTimeEdit *>(this->ui->tblCaption->cellWidget(nId, 2));
        if (ptimeedtEnd == nullptr) continue;
        ptimeedtEnd->setMinimumTime(this->m_videoIntro.time);
        ptimeedtEnd->setMaximumTime(this->m_timeMid);
    }

    this->updateCaptionList();
}

void QUIMain::on_sldTime_valueChanged(int value)
{
    // calculate current time
    this->m_nMsecCur = int(double(this->m_nMsecTotal) / 10000.0 * double(value));
    this->m_timeCur = QTime::fromMSecsSinceStartOfDay(this->m_nMsecCur);
    this->ui->lblTimeCur->setText(this->m_timeCur.toString("hh:mm:ss.zzz"));

    this->us_updatePreview();
}

bool QUIMain::grabFrame(Video *pVideo, QTime *pTimeCur, QPixmap *ppixFrame)
{
    // calc frame number
    int nMsec = abs(pTimeCur->msecsTo(QTime(0, 0, 0, 0)));
    int nFrameNumber = int(double(nMsec) * pVideo->fFPS / 1000.0);

    if (nFrameNumber > pVideo->nFrameCount) {
        qDebug("grabFrame [%s] - error - frame number > frame count", pVideo->strFile.toUtf8().data());
        return false;
    }

    // read current frame
    cv::VideoCapture capVideo(pVideo->strFile.toStdString());
    if (!capVideo.isOpened()) {
        qDebug("grabFrame [%s] - video_capture - open error", pVideo->strFile.toUtf8().data());
        pVideo->bAvailable = false;
        return false;
    }

    capVideo.set(cv::CAP_PROP_POS_FRAMES, nFrameNumber);

    cv::Mat matFrame;
    bool bResult = capVideo.read(matFrame);
    if (!bResult) {
        qDebug("grabFrame [%s] - video_capture - read error", pVideo->strFile.toUtf8().data());
        pVideo->bAvailable = false;
        return false;
    }

    capVideo.release();

    *ppixFrame = QCV::cvMatToQPixmap(matFrame);

    return true;
}

void QUIMain::initPreviewVideo()
{
    this->ui->lblVideo->clear();
    this->ui->lblVideo->setText("Not Available");
    this->m_pLogoPreview->hide();
    this->ui->lblTimeCur->setText("00:00:00.000");
    this->ui->lblTimeTotal->setText("00:00:00.000");
    this->ui->sldTime->setValue(0);
    this->ui->btnPlay->setEnabled(false);
    this->ui->sldTime->setEnabled(false);
    this->ui->btnRender->setEnabled(false);
}

QTime QUIMain::timeDiff(QTime *pTime1, QTime *pTime2)
{
    int nMsec = abs(pTime1->msecsTo(*pTime2));
    int nSecond = nMsec / 1000;
    nMsec %= 1000;
    int nMinute = nSecond / 60;
    nSecond %= 60;
    int nHour = nMinute / 60;
    nMinute %= 60;

    QTime time(nHour, nMinute, nSecond, nMsec);
    return time;
}

QTime QUIMain::timeAdd(QTime *pTime1, QTime *pTime2)
{
    int nMsecTime1 = abs(pTime1->msecsTo(QTime(0, 0, 0, 0)));
    QTime timeSum = pTime2->addMSecs(nMsecTime1);
    int nMsec = abs(timeSum.msecsTo(QTime(0, 0, 0, 0)));
    int nSecond = nMsec / 1000;
    nMsec %= 1000;
    int nMinute = nSecond / 60;
    nSecond %= 60;
    int nHour = nMinute / 60;
    nMinute %= 60;

    QTime time(nHour, nMinute, nSecond, nMsec);
    return time;
}

void QUIMain::drawCaption(QPixmap *ppixFrame, QString strCaption, QString strImage)
{
    QFontMetrics fontMetrics(this->m_fontCaption);
    int nLineSpacing = fontMetrics.lineSpacing();

    // line count
    strCaption = strCaption.trimmed();
    QStringList strlstCaption = strCaption.split("\n");
    int nLineCount = strlstCaption.length();

    // caption text area
    int nCaptionTextAreaWidth = ppixFrame->width() - N_CAPTION_TEXT_MARGIN_LEFT -
                                N_CAPTION_TEXT_MARGIN_RIGHT;
    int nCaptionTextAreaHeight = nLineCount * nLineSpacing;

    QRect rectCaptionTextArea(N_CAPTION_TEXT_MARGIN_LEFT,
                              ppixFrame->height() - N_CAPTION_TEXT_MARGIN_BOTTOM - nCaptionTextAreaHeight,
                              nCaptionTextAreaWidth,
                              nCaptionTextAreaHeight);

    // bounding area
    int nBoundingWidth = ppixFrame->width();
    int nBoundingHeight = N_CAPTION_TEXT_MARGIN_BOTTOM + nCaptionTextAreaHeight +
                          N_CAPTION_TEXT_MARGIN_TOP;
    QRect rectBounding(0,
                       ppixFrame->height() - nBoundingHeight,
                       nBoundingWidth,
                       nBoundingHeight);

    QPainter painter( ppixFrame );
//    painter.drawRect(rectBounding);

    if (strImage == "white") {
        painter.fillRect(rectBounding, QColor(255, 255, 255));

        QPen penUpperLine(QBrush(QColor(23, 86, 129)), 7, Qt::SolidLine);
        painter.setPen(penUpperLine);
        painter.drawLine(rectBounding.topLeft(), rectBounding.topRight());
    } else {
        painter.drawImage(rectBounding, QImage(strImage));
    }

    QPen penCaption(Qt::black, 2, Qt::DashLine);
    painter.setPen(penCaption);
    painter.setFont(this->m_fontCaption);

    QFont fontSmall = this->m_fontCaption;
    fontSmall.setPointSize(int(double(fontSmall.pointSize()) * 15.0 / 17.0));

    // draw text
    for (int nId = 0 ; nId < nLineCount ; nId ++) {
        if (nId > 0) {
            painter.setFont(fontSmall);
        }

        QRect rectCaption(N_CAPTION_TEXT_MARGIN_LEFT,
                          rectCaptionTextArea.top() + nId * nLineSpacing,
                          nCaptionTextAreaWidth,
                          nLineSpacing);
        painter.drawText(rectCaption, Qt::TextWordWrap | Qt::AlignHCenter, strlstCaption.at(nId));
    }

//    painter.drawRect(rectCaptionTextArea);
}

void QUIMain::drawLogo(QPixmap *ppixFrame)
{
    int nCurVideo = this->ui->tblBatch->currentRow();
    if (nCurVideo == -1) return;
    if (this->m_pixLogo.width() == 0) return;

    int nLogoLeft = int(double(this->m_nLogoX * this->m_lstBatch.at(nCurVideo).nWidth) / double(this->m_rectVideoArea.width()));
    int nLogoTop = int(double(this->m_nLogoY * this->m_lstBatch.at(nCurVideo).nHeight) / double(this->m_rectVideoArea.height()));
    int nLogoWidth = int(double(this->m_pLogoPreview->width() * this->m_lstBatch.at(nCurVideo).nWidth) / double(this->m_rectVideoArea.width()));
    int nLogoHeight = int(double(this->m_pLogoPreview->height() * this->m_lstBatch.at(nCurVideo).nHeight) / double(this->m_rectVideoArea.height()));

    QPainter painter( ppixFrame );
    painter.drawImage(QRect(nLogoLeft, nLogoTop, nLogoWidth, nLogoHeight), this->m_pixLogo.toImage());
}

void QUIMain::on_btnPlay_clicked()
{
    if (this->m_bIsPlaying) {
        if (g_pThreadPlay && g_pThreadPlay->isRunning()) {
            g_pThreadPlay->stop();
        }
    } else {
        if (!this->canPlay()) return;

        this->m_bIsPlaying = true;
        this->ui->btnPlay->setText("Stop");
        this->m_pLogoPreview->hide();

        // freeze ui
        this->setEnabledUIPlay(false);

        this->ui->sldTime->disconnect(SIGNAL(valueChanged(int)));

        // update video area & logo preview
        int nCurVideo = this->ui->tblBatch->currentRow();
        this->updateVideoAreaLogo(nCurVideo);

        // update caption list
        this->updateCaptionList();

        g_pThreadPlay = new QThreadPlay(this, this->ui->tblBatch->currentRow());
        this->connect(g_pThreadPlay, SIGNAL(playFrame(int, QPixmap, int)), this, SLOT(on_playFrame(int, QPixmap, int)));
        this->connect(g_pThreadPlay, SIGNAL(finished()), this, SLOT(us_threadPlay_finished()));
        g_pThreadPlay->start();
    }
}

void QUIMain::on_playFrame(int nFrameSrc, QPixmap pixFrame, int nFrameNumber)
{
    // get preview frame width & height
    QSize szPreview = this->ui->lblVideo->size();

    if (nFrameSrc == N_FRAME_SRC_INTRO) {
        // scale to preview frame
        QPixmap pixVideoFrameFit = pixFrame.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblVideo->setPixmap(pixVideoFrameFit);

        // correct current time & slider
        this->m_timeCur = this->calcTime(this->m_videoIntro.fFPS, nFrameNumber);
        this->m_nMsecCur = abs(this->m_timeCur.msecsTo(QTime(0, 0, 0, 0)));

        int nValue = int(double(this->m_nMsecCur) * 100.0 / (double(this->m_nMsecTotal) / 100.0));
        this->ui->sldTime->setValue(nValue);
        this->ui->lblTimeCur->setText(this->m_timeCur.toString("hh:mm:ss.zzz"));
    } else if (nFrameSrc == N_FRAME_SRC_OUTRO) {
        // scale to preview frame
        QPixmap pixVideoFrameFit = pixFrame.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblVideo->setPixmap(pixVideoFrameFit);

        // correct current time & slider
        QTime timeOutro = this->calcTime(this->m_videoOutro.fFPS, nFrameNumber);
        int nMsecOurto = abs(timeOutro.msecsTo(QTime(0, 0, 0, 0)));
        this->m_timeCur = this->m_timeMid.addMSecs(nMsecOurto);
        this->m_nMsecCur = abs(this->m_timeCur.msecsTo(QTime(0, 0, 0, 0)));

        int nValue = int(double(this->m_nMsecCur) * 100.0 / (double(this->m_nMsecTotal) / 100.0));
        this->ui->sldTime->setValue(nValue);
        this->ui->lblTimeCur->setText(this->m_timeCur.toString("hh:mm:ss.zzz"));
    } else if (nFrameSrc == N_FRAME_SRC_BATCH) {
        // draw caption
        int nCaptionCount = this->m_lstCaption.length();
        for (int nId = 0 ; nId < nCaptionCount ; nId ++) {
            if (this->m_timeCur < this->m_lstCaption.at(nId).timeStart) continue;
            if (this->m_timeCur > this->m_lstCaption.at(nId).timeEnd) continue;

            // draw the first caption
            this->drawCaption(&pixFrame, this->m_lstCaption.at(nId).strCaption, this->m_lstCaption.at(nId).strImage);
            break;
        }

        // draw logo
        this->drawLogo(&pixFrame);

        // scale to preview frame
        QPixmap pixVideoFrameFit = pixFrame.scaled(szPreview, Qt::KeepAspectRatio);
        this->ui->lblVideo->setPixmap(pixVideoFrameFit);

        // correct current time & slider
        int nCurVideo = this->ui->tblBatch->currentRow();
        QTime timeBatch = this->calcTime(this->m_lstBatch.at(nCurVideo).fFPS, nFrameNumber);
        int nMsecBatch = abs(timeBatch.msecsTo(QTime(0, 0, 0, 0)));
        this->m_timeCur = this->m_videoIntro.time.addMSecs(nMsecBatch);
        this->m_nMsecCur = abs(this->m_timeCur.msecsTo(QTime(0, 0, 0, 0)));

        int nValue = int(double(this->m_nMsecCur) * 100.0 / (double(this->m_nMsecTotal) / 100.0));
        this->ui->sldTime->setValue(nValue);
        this->ui->lblTimeCur->setText(this->m_timeCur.toString("hh:mm:ss.zzz"));
    }
}

void QUIMain::updateVideoAreaLogo(int nVideo)
{
    // get preview frame width & height
    QSize szPreview = this->ui->lblVideo->size();

    // scale to preview frame
    QPixmap pixVideoFrameFit = this->m_lstBatch.at(nVideo).pixPreview.scaled(szPreview, Qt::KeepAspectRatio);

    // calculate video frame area
    QRect rectVideoArea;
    if (pixVideoFrameFit.width() < szPreview.width()) {
        rectVideoArea.setLeft(int(double(szPreview.width() - pixVideoFrameFit.width()) / 2.0));
        rectVideoArea.setTop(0);
    } else {
        rectVideoArea.setLeft(0);
        rectVideoArea.setTop(int(double(szPreview.height() - pixVideoFrameFit.height()) / 2.0));
    }

    rectVideoArea.setWidth(pixVideoFrameFit.width());
    rectVideoArea.setHeight(pixVideoFrameFit.height());

    this->m_rectVideoArea = rectVideoArea;

    // logo
    if (this->m_pixLogo.width() != 0 && this->m_nLogoWidth == 0) {
        this->m_nLogoWidth = int(double(this->m_pixLogo.width() * rectVideoArea.width()) / double(this->m_lstBatch.at(nVideo).nWidth));
        this->m_nLogoHeight = int(double(this->m_pixLogo.height() * rectVideoArea.height()) / double(this->m_lstBatch.at(nVideo).nHeight));

        // arrange logo on preview frame
        this->m_pLogoPreview->setGeometry(
            rectVideoArea.left() + this->m_nLogoX,
            rectVideoArea.top() + this->m_nLogoY,
            this->m_nLogoWidth,
            this->m_nLogoHeight);
    }
}

void QUIMain::us_threadPlay_finished()
{
    this->m_bIsPlaying = false;
    this->ui->btnPlay->setText("Play");

    if (this->m_pixLogo.width() != 0 && this->m_timeCur > this->m_videoIntro.time && this->m_timeCur <= this->m_timeMid) {
        this->m_pLogoPreview->show();
    }

    this->ui->lblTimeCur->setText(this->ui->lblTimeTotal->text());

    // unfreeze ui
    this->setEnabledUIPlay(true);

    this->us_updatePreview();

    this->connect(this->ui->sldTime, SIGNAL(valueChanged(int)), this, SLOT(on_sldTime_valueChanged(int)));
}

void QUIMain::on_btnRender_clicked()
{
    if (this->m_bIsRendering) {
        if (g_pThreadVD && g_pThreadVD->isRunning()) {
            g_pThreadVD->stop();
        }

        if (g_pThreadFFMpeg && g_pThreadFFMpeg->isRunning()) {
            g_pThreadFFMpeg->stop();
        }
    } else {
        this->m_bIsRendering = true;
        this->initPreviewPanel();

        this->ui->btnRender->setText("Stop");
        this->m_pLogoPreview->hide();

        // freeze ui
        this->setEnabledUIRender(false);

        // update caption list
        this->updateCaptionList();

        // start rendering
        this->m_nCurRendering = -1;
        this->m_fTotalProgStep = 100.0 / double(this->m_lstBatch.length());
        this->ui->prgStage->setValue(0);
        this->ui->prgUnit->setValue(0);
        this->ui->prgTotal->setValue(0);

        // disconnect
        this->ui->tblBatch->disconnect(SIGNAL(cellChanged(int, int)));
        this->ui->tblBatch->disconnect(SIGNAL(currentCellChanged(int, int, int, int)));

        this->on_stageFinished(N_STAGE_ID_MA, N_STAGE_RET_SUCCESS);
    }
}

void QUIMain::updateCaptionList()
{
    this->m_lstCaption.clear();
    int nCaptionCount = this->ui->tblCaption->rowCount();
    for (int nId = 0 ; nId < nCaptionCount ; nId ++) {
        // caption string
        QTextEdit *ptextedtCaption = static_cast<QTextEdit *>(this->ui->tblCaption->cellWidget(nId, 3));
        if (ptextedtCaption == nullptr) continue;
        QString strCaption = ptextedtCaption->toPlainText();
        if (strCaption.isEmpty()) continue;

        // start time
        QTimeEdit *ptimeedtStart = static_cast<QTimeEdit *>(this->ui->tblCaption->cellWidget(nId, 1));
        if (ptimeedtStart == nullptr) continue;
        QTime timeStart = ptimeedtStart->time();

        // end time
        QTimeEdit *ptimeedtEnd = static_cast<QTimeEdit *>(this->ui->tblCaption->cellWidget(nId, 2));
        if (ptimeedtEnd == nullptr) continue;
        QTime timeEnd = ptimeedtEnd->time();

        if (timeStart == timeEnd) continue;

        // background image
        QItemCB *pItemCB = static_cast<QItemCB *>(this->ui->tblCaption->cellWidget(nId, 4));
        QString strImage = pItemCB->value();

        Caption caption;
        caption.strCaption = strCaption;
        caption.timeStart = timeStart;
        caption.timeEnd = timeEnd;
        caption.strImage = strImage;

        this->m_lstCaption.append(caption);
    }
}

QString QUIMain::addFileSuffix(QString strFolder, QString strFile, QString strSuffix)
{
    QFileInfo fileInfo(strFile);
    QString strExt = fileInfo.suffix();
    QString strNewFile = strFolder + strFile.left(strFile.length() - strExt.length() - 1) + strSuffix + ".avi";
    return strNewFile;
}

void QUIMain::on_stagePercentChanged(QString strStageTitle, int nPercent)
{
    this->ui->lblStage->setText(strStageTitle);
    this->ui->prgStage->setValue(nPercent);
}

void QUIMain::on_stageFinished(int nStageId, int nRet)
{
//    qDebug("on_stageFinished - %d : %d", nStageId, nRet);

    // check if user clicked <stop> button
    if (nRet == N_STAGE_RET_STOPPED) {
        qDebug("rendering finished - stopped");
        this->ui->lblUnit->setText("[ " + this->m_lstBatch.at(this->m_nCurRendering).strTitle + " ] processing was stopped.");
        this->ui->lblTotal->setText("total progressing was stopped.");
        this->restoreFromRendering();
        return;
    }

    // check if rendering has reached the last stage with error
    if (nRet == N_STAGE_RET_ERROR && this->m_nCurRendering == this->m_lstBatch.length() - 1) {
        qDebug("rendering finished - error occured at the last file");

        this->ui->lblUnit->setText("[ " + this->m_lstBatch.at(this->m_nCurRendering).strTitle + " ] processing has been finished.");
        this->ui->lblTotal->setText("total progressing has been finished.");
        this->ui->prgTotal->setValue(100);
        this->restoreFromRendering();
        return;
    }

    // check if rendering has reached the last stage successfully
    if (nStageId == N_STAGE_ID_MA && this->m_nCurRendering == this->m_lstBatch.length() - 1) {
        qDebug("rendering finished - rendering has reached the last stage successfully.");

        QFile::remove(this->addFileSuffix(this->m_strFolderTmp, this->m_lstBatch.at(this->m_nCurRendering).strTitle, "_av"));
        QFile::remove(this->addFileSuffix(this->m_strFolderTmp, this->m_videoIntro.strTitle, "_ci"));
        QFile::remove(this->addFileSuffix(this->m_strFolderTmp, this->m_videoOutro.strTitle, "_co"));

        this->ui->lblUnit->setText("[ " + this->m_lstBatch.at(this->m_nCurRendering).strTitle +
                                   " ] processing has been finished successfully.");
        this->ui->prgUnit->setValue(100);
        this->ui->lblTotal->setText("total progressing has been finished successfully.");
        this->ui->prgTotal->setValue(100);
        this->restoreFromRendering();
        return;
    }

    // check if error occured
    if (nRet == N_STAGE_RET_ERROR) {
        nStageId = N_STAGE_ID_MA;
    }

    if (nStageId == N_STAGE_ID_AD) {
        this->ui->prgUnit->setValue(17);
        qDebug("%s - ad - ended", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());

        //////////////////////////////////////////////////////////////
        QString strOutput = this->addFileSuffix(this->m_strFolderTmp, this->m_lstBatch.at(this->m_nCurRendering).strTitle, "_vd");
        QFile::remove(strOutput);
        g_pThreadVD = new QThreadVD(this, this->m_nCurRendering, strOutput);
        this->connect(g_pThreadVD, SIGNAL(stagePercentChanged(QString, int)), this,
                      SLOT(on_stagePercentChanged(QString, int)));
        this->connect(g_pThreadVD, SIGNAL(stageFinished(int, int)), this, SLOT(on_stageFinished(int, int)));
        g_pThreadVD->start();
        qDebug("%s - vd - started", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
    }

    if (nStageId == N_STAGE_ID_VD) {
        this->ui->prgUnit->setValue(33);
        qDebug("%s - vd - ended", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());

        /////////////////////////////////////////////////////////////
        Stage stageAV;
        stageAV.nStageId = N_STAGE_ID_AV;
        stageAV.nTotalMsec = abs(this->m_lstBatch.at(this->m_nCurRendering).time.msecsTo(QTime(0, 0, 0,
                                                                                               0)));
        stageAV.strTitleProcessing = "av processing ...";
        stageAV.strTitleSuccess = "av processing has been finished successfully.";
        stageAV.strTitleError = "av processing has been finished.";
        stageAV.strTitleStopped = "av processing was stopped.";
        stageAV.strOutput = this->addFileSuffix(this->m_strFolderTmp,
                                                this->m_lstBatch.at(this->m_nCurRendering).strTitle, "_av");
        QFile::remove(stageAV.strOutput);
        // ffmpeg.exe -i algol_vd.mp4 -i algol.mp3 -r 30 -vf scale=3840:2160 -vcodec h264 -acodec mp3 algol_av.avi
        stageAV.strlstArguments << "-i"
                                << this->addFileSuffix(this->m_strFolderTmp, this->m_lstBatch.at(this->m_nCurRendering).strTitle,
                                                       "_vd")
                                << "-i"
                                << this->m_strFolderTmp + this->m_lstBatch.at(this->m_nCurRendering).strTitle + ".mp3"
                                << "-r"
                                << this->m_strCurFPS
                                << "-vf"
                                << "scale=" + this->m_strCurScale
                                << "-vcodec"
                                << "h264"
                                << "-acodec"
                                << "mp3"
                                << "-y"
                                << stageAV.strOutput;

        g_pThreadFFMpeg = new QThreadFFMpeg(stageAV);
        this->connect(g_pThreadFFMpeg, SIGNAL(stagePercentChanged(QString, int)), this, SLOT(on_stagePercentChanged(QString, int)));
        this->connect(g_pThreadFFMpeg, SIGNAL(stageFinished(int, int)), this, SLOT(on_stageFinished(int, int)));
        g_pThreadFFMpeg->start(this->m_ThreadPriority);
        qDebug("%s - av - started", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
    }

    if (nStageId == N_STAGE_ID_AV) {
        QFile::remove(this->m_strFolderTmp + this->m_lstBatch.at(this->m_nCurRendering).strTitle + ".mp3");
        QFile::remove(this->addFileSuffix(this->m_strFolderTmp, this->m_lstBatch.at(this->m_nCurRendering).strTitle, "_vd"));
        this->ui->prgUnit->setValue(50);
        qDebug("%s - av - ended", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());

        /////////////////////////////////////////////////////////////
        if (this->m_videoIntro.bAvailable) {
            Stage stageCI;
            stageCI.nStageId = N_STAGE_ID_CI;
            stageCI.nTotalMsec = abs(this->m_videoIntro.time.msecsTo(QTime(0, 0, 0, 0)));
            stageCI.strTitleProcessing = "ci processing ...";
            stageCI.strTitleSuccess = "ci processing has been finished successfully.";
            stageCI.strTitleError = "ci processing has been finished.";
            stageCI.strTitleStopped = "ci processing was stopped.";
            stageCI.strOutput = this->addFileSuffix(this->m_strFolderTmp, this->m_videoIntro.strTitle, "_ci");
            QFile::remove(stageCI.strOutput);
            // ffmpeg.exe -i intro.mp4 -r 30 -vf scale=3840:2160 -vcodec h264 -acodec mp3 intro_ci.avi
            stageCI.strlstArguments << "-i"
                                    << this->m_videoIntro.strFile
                                    << "-r"
                                    << this->m_strCurFPS
                                    << "-vf"
                                    << "scale=" + this->m_strCurScale
                                    << "-vcodec"
                                    << "h264"
                                    << "-acodec"
                                    << "mp3"
                                    << "-y"
                                    << stageCI.strOutput;

            g_pThreadFFMpeg = new QThreadFFMpeg(stageCI);
            this->connect(g_pThreadFFMpeg, SIGNAL(stagePercentChanged(QString, int)), this, SLOT(on_stagePercentChanged(QString, int)));
            this->connect(g_pThreadFFMpeg, SIGNAL(stageFinished(int, int)), this, SLOT(on_stageFinished(int, int)));
            g_pThreadFFMpeg->start(this->m_ThreadPriority);
            qDebug("%s - ci - started", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
        } else {
            nStageId = N_STAGE_ID_CI;
        }
    }

    if (nStageId == N_STAGE_ID_CI) {
        this->ui->prgUnit->setValue(67);
        if (this->m_videoIntro.bAvailable) {
            qDebug("%s - ci - ended", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
        }

        /////////////////////////////////////////////////////////////
        if (this->m_videoOutro.bAvailable) {
            Stage stageCO;
            stageCO.nStageId = N_STAGE_ID_CO;
            stageCO.nTotalMsec = abs(this->m_videoOutro.time.msecsTo(QTime(0, 0, 0, 0)));
            stageCO.strTitleProcessing = "co processing ...";
            stageCO.strTitleSuccess = "co processing has been finished successfully.";
            stageCO.strTitleError = "co processing has been finished.";
            stageCO.strTitleStopped = "co processing was stopped.";
            stageCO.strOutput = this->addFileSuffix(this->m_strFolderTmp, this->m_videoOutro.strTitle, "_co");
            QFile::remove(stageCO.strOutput);
            // ffmpeg.exe -i outro.mp4 -r 30 -vf scale=3840:2160 -vcodec h264 -acodec mp3 outro_co.avi
            stageCO.strlstArguments << "-i"
                                    << this->m_videoOutro.strFile
                                    << "-r"
                                    << this->m_strCurFPS
                                    << "-vf"
                                    << "scale=" + this->m_strCurScale
                                    << "-vcodec"
                                    << "h264"
                                    << "-acodec"
                                    << "mp3"
                                    << "-y"
                                    << stageCO.strOutput;

            g_pThreadFFMpeg = new QThreadFFMpeg(stageCO);
            this->connect(g_pThreadFFMpeg, SIGNAL(stagePercentChanged(QString, int)), this,
                          SLOT(on_stagePercentChanged(QString, int)));
            this->connect(g_pThreadFFMpeg, SIGNAL(stageFinished(int, int)), this, SLOT(on_stageFinished(int,
                                                                                                        int)));
            g_pThreadFFMpeg->start(this->m_ThreadPriority);
            qDebug("%s - co - started", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
        } else {
            nStageId = N_STAGE_ID_CO;
        }
    }

    if (nStageId == N_STAGE_ID_CO) {
        this->ui->prgUnit->setValue(83);
        if (this->m_videoOutro.bAvailable) {
            qDebug("%s - co - ended", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
        }

        //////////////////////////////////////////////////////////////
        QTime timeIntro = this->m_videoIntro.time;
        QTime timeVideo = this->m_lstBatch.at(this->m_nCurRendering).time;
        QTime timeOutro = this->m_videoOutro.time;

        QTime timeSum = timeVideo;
        if (this->m_videoIntro.bAvailable) {
            timeSum = this->timeAdd(&timeIntro, &timeVideo);
        }
        if (this->m_videoOutro.bAvailable) {
            QTime timeIV = timeSum;
            timeSum = this->timeAdd(&timeIV, &timeOutro);
        }

        Stage stageMA;
        stageMA.nStageId = N_STAGE_ID_MA;
        stageMA.nTotalMsec = abs(timeSum.msecsTo(QTime(0, 0, 0, 0)));
        stageMA.strTitleProcessing = "ma processing ...";
        stageMA.strTitleSuccess = "ma processing has been finished successfully.";
        stageMA.strTitleError = "ma processing has been finished.";
        stageMA.strTitleStopped = "ma processing was stopped.";
        stageMA.strOutput = this->addFileSuffix(this->m_strFolderOut, this->m_lstBatch.at(this->m_nCurRendering).strTitle, "_ma");
        QFile::remove(stageMA.strOutput);
        // ffmpeg -i intro_ci.avi -i algol_av.avi -i outro_co.avi -vcodec h264 -filter_complex "[0:v] [0:a] [1:v] [1:a] [2:v] [2:a] concat=n=3:v=1:a=1 [v] [a]" -map "[v]" -map "[a]" algol_ma.avi

        int nMergeCount = 0;
        if (this->m_videoIntro.bAvailable) {
            stageMA.strlstArguments << "-i"
                                    << this->addFileSuffix(this->m_strFolderTmp, this->m_videoIntro.strTitle, "_ci");
            nMergeCount ++;
        }

        stageMA.strlstArguments << "-i"
                                << this->addFileSuffix(this->m_strFolderTmp, this->m_lstBatch.at(this->m_nCurRendering).strTitle, "_av");
        nMergeCount ++;

        if (this->m_videoOutro.bAvailable) {
            stageMA.strlstArguments << "-i"
                                    << this->addFileSuffix(this->m_strFolderTmp, this->m_videoOutro.strTitle, "_co");
            nMergeCount ++;
        }

        stageMA.strlstArguments << "-vcodec"
                                << "h264"
                                << "-filter_complex";

        QString strFilterComplex;
        for (int nId = 0; nId < nMergeCount; nId ++) {
            QString strId = QString::number(nId, 10);
            strFilterComplex += "[" + strId + ":v] [" + strId + ":a] ";
        }
        strFilterComplex += "concat=n=";
        QString strMergeCount = QString::number(nMergeCount, 10);
        strFilterComplex += strMergeCount;
        strFilterComplex += ":v=1:a=1 [v] [a]";

        stageMA.strlstArguments << strFilterComplex
                                << "-map"
                                << "[v]"
                                << "-map"
                                << "[a]"
                                << "-y"
                                << stageMA.strOutput;

        g_pThreadFFMpeg = new QThreadFFMpeg(stageMA);
        this->connect(g_pThreadFFMpeg, SIGNAL(stagePercentChanged(QString, int)), this,
                      SLOT(on_stagePercentChanged(QString, int)));
        this->connect(g_pThreadFFMpeg, SIGNAL(stageFinished(int, int)), this, SLOT(on_stageFinished(int,
                                                                                                    int)));
        g_pThreadFFMpeg->start(this->m_ThreadPriority);
        qDebug("%s - ma - started", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
    }

    if (nStageId == N_STAGE_ID_MA) {
        if (this->m_nCurRendering == -1) {
            qDebug("rendering started.");
            this->ui->lblTotal->setText("total progressing ...");
        } else {
            QFile::remove(this->addFileSuffix(this->m_strFolderTmp, this->m_lstBatch.at(this->m_nCurRendering).strTitle, "_av"));
            QFile::remove(this->addFileSuffix(this->m_strFolderTmp, this->m_videoIntro.strTitle, "_ci"));
            QFile::remove(this->addFileSuffix(this->m_strFolderTmp, this->m_videoOutro.strTitle, "_co"));

            this->ui->lblUnit->setText("[ " + this->m_lstBatch.at(this->m_nCurRendering).strTitle +
                                       " ] processing has been finished.");
            this->ui->prgUnit->setValue(100);
            QThread::msleep(500);
            qDebug("%s - ma - ended", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());
        }

        this->ui->prgTotal->setValue(int(this->m_fTotalProgStep * (this->m_nCurRendering + 1)));
        this->m_nCurRendering ++;

        this->m_strCurFPS = QString::number(this->m_lstBatch.at(this->m_nCurRendering).fFPS);
        this->m_strCurScale = QString::number(this->m_lstBatch.at(this->m_nCurRendering).nWidth, 10) +
                              ":" +
                              QString::number(this->m_lstBatch.at(this->m_nCurRendering).nHeight, 10);

        QString strUnitTitle = "[ " + this->m_lstBatch.at(this->m_nCurRendering).strTitle +
                               " ] processing...";
        this->ui->lblUnit->setText(strUnitTitle);

        // current rendering
        this->ui->tblBatch->selectRow(this->m_nCurRendering);

        Stage stageAD;
        stageAD.nStageId = N_STAGE_ID_AD;
        stageAD.nTotalMsec = abs(this->m_lstBatch.at(this->m_nCurRendering).time.msecsTo(QTime(0, 0, 0, 0)));
        stageAD.strTitleProcessing = "ad processing ...";
        stageAD.strTitleSuccess = "ad processing has been finished successfully.";
        stageAD.strTitleError = "ad processing has been finished.";
        stageAD.strTitleStopped = "ad processing was stopped.";
        stageAD.strOutput = this->m_strFolderTmp + this->m_lstBatch.at(this->m_nCurRendering).strTitle + ".mp3";
        // ffmpeg.exe -i algol.mp4 -f mp3 -ab 192000 -vn -y algol.mp4.mp3
        stageAD.strlstArguments << "-i"
                                << this->m_lstBatch.at(this->m_nCurRendering).strFile
                                << "-f"
                                << "mp3"
                                << "-ab"
                                << "192000"
                                << "-vn"
                                << "-y"
                                << stageAD.strOutput;

        g_pThreadFFMpeg = new QThreadFFMpeg(stageAD);
        this->connect(g_pThreadFFMpeg, SIGNAL(stagePercentChanged(QString, int)), this, SLOT(on_stagePercentChanged(QString, int)));
        this->connect(g_pThreadFFMpeg, SIGNAL(stageFinished(int, int)), this, SLOT(on_stageFinished(int, int)));
        g_pThreadFFMpeg->start(this->m_ThreadPriority);
        qDebug("%s - ad - started", this->m_lstBatch.at(this->m_nCurRendering).strTitle.toUtf8().data());

        this->ui->prgUnit->setValue(0);
    }
}

void QUIMain::restoreFromRendering()
{
    this->m_bIsRendering = false;
    this->ui->btnRender->setText("Render");

    // connect signals
    this->connect(this->ui->tblBatch, SIGNAL(cellChanged(int, int)), SLOT(us_tbl_cellChanged_Batch(int, int)));
    this->connect(this->ui->tblBatch, SIGNAL(currentCellChanged(int, int, int, int)), SLOT(on_tblBatch_currentCellChanged(int, int, int, int)));

    // unfreeze ui
    this->setEnabledUIRender(true);
}

void QUIMain::us_logo_mouseMove()
{
    if (this->m_bIsLogoMoving) {
        QPoint ptCur = QCursor::pos();

        int nDx = ptCur.x() - this->m_ptPrev.x();
        int nDy = ptCur.y() - this->m_ptPrev.y();

        QRect rectLogo = this->m_pLogoPreview->geometry();
        QRect rectNew = rectLogo;

        bool bResize = false;
        if (this->m_nLogoMouse == N_LOGO_MOUSE_LEFT) {
            // left
            int nLeftNew = rectLogo.left() + nDx;
            if (nLeftNew < this->m_rectVideoArea.left()) {
                nLeftNew = this->m_rectVideoArea.left();
            } else if (nLeftNew + N_LOGO_MIN_SIZE >= rectLogo.right()) {
                nLeftNew = rectLogo.right() - N_LOGO_MIN_SIZE;
            }

            rectNew.setLeft(nLeftNew);
            int nWidth = rectNew.width();
            int nHeightNew = int(double(nWidth) / this->m_fLogoRatio);
            rectNew.setHeight(nHeightNew);
            if (rectNew.top() >= this->m_rectVideoArea.top()
                    && rectNew.bottom() <= (this->m_rectVideoArea.bottom() + 2)) {
                this->m_pLogoPreview->setGeometry(rectNew);
            }

            bResize = true;
        } else if (this->m_nLogoMouse == N_LOGO_MOUSE_TOP) {
            // top
            int nTopNew = rectLogo.top() + nDy;
            if (nTopNew < this->m_rectVideoArea.top()) {
                nTopNew = this->m_rectVideoArea.top();
            } else if (nTopNew + N_LOGO_MIN_SIZE >= rectLogo.bottom()) {
                nTopNew = rectLogo.bottom() - N_LOGO_MIN_SIZE;
            }
            rectNew.setTop(nTopNew);
            int nHeight = rectNew.height();
            int nWidthNew = int (nHeight * this->m_fLogoRatio);
            rectNew.setWidth(nWidthNew);
            if (rectNew.left() >= this->m_rectVideoArea.left()
                    && rectNew.right() <= (this->m_rectVideoArea.right() + 2)) {
                this->m_pLogoPreview->setGeometry(rectNew);
            }
            bResize = true;
        } else if (this->m_nLogoMouse == N_LOGO_MOUSE_RIGHT) {
            // right
            int nRightNew = rectLogo.right() + nDx;
            if (nRightNew >= this->m_rectVideoArea.right() + 3) {
                nRightNew = this->m_rectVideoArea.right() + 3;
            } else if (nRightNew < rectLogo.left() + N_LOGO_MIN_SIZE) {
                nRightNew = rectLogo.left() + N_LOGO_MIN_SIZE;
            }
            rectNew.setRight(nRightNew);
            int nWidth = rectNew.width();
            int nHeightNew = int(double(nWidth) / this->m_fLogoRatio);
            rectNew.setHeight(nHeightNew);
            if (rectNew.top() >= this->m_rectVideoArea.top()
                    && rectNew.bottom() <= (this->m_rectVideoArea.bottom() + 2)) {
                this->m_pLogoPreview->setGeometry(rectNew);
            }
            bResize = true;
        } else if (this->m_nLogoMouse == N_LOGO_MOUSE_BOTTOM) {
            // bottom
            int nBottomNew = rectLogo.bottom() + nDy;
            if (nBottomNew >= this->m_rectVideoArea.bottom() + 3) {
                nBottomNew = this->m_rectVideoArea.bottom() + 3;
            } else if (nBottomNew < rectLogo.top() + N_LOGO_MIN_SIZE) {
                nBottomNew = rectLogo.top() + N_LOGO_MIN_SIZE;
            }
            rectNew.setBottom(nBottomNew);
            int nHeight = rectNew.height();
            int nWidthNew = int (nHeight * this->m_fLogoRatio);
            rectNew.setWidth(nWidthNew);
            if (rectNew.left() >= this->m_rectVideoArea.left()
                    && rectNew.right() <= (this->m_rectVideoArea.right() + 2)) {
                this->m_pLogoPreview->setGeometry(rectNew);
            }
            bResize = true;
        }

        if (bResize) {
            // resizing
            this->m_nLogoX = rectNew.left() -  this->m_rectVideoArea.left();
            this->m_nLogoY = rectNew.top() - this->m_rectVideoArea.top();
            this->m_nLogoWidth = rectNew.width();
            this->m_nLogoHeight = rectNew.height();
        } else if (!bResize && this->m_nLogoMouse == N_LOGO_MOUSE_INSIDE) {
            // moving
            // check if log-preview goes over the video area and recalculate new pos
            QPoint ptLogo = this->m_pLogoPreview->pos();
            int nNewPosX = ptLogo.x() + nDx;
            int nNewPosY = ptLogo.y() + nDy;

            if (nNewPosX < this->m_rectVideoArea.left()) {
                nNewPosX = this->m_rectVideoArea.left();
            } else if (nNewPosX + this->m_pLogoPreview->width() >= this->m_rectVideoArea.right() + 2) {
                nNewPosX = this->m_rectVideoArea.right() - this->m_pLogoPreview->width() + 2;
            }

            if (nNewPosY < this->m_rectVideoArea.top()) {
                nNewPosY = this->m_rectVideoArea.top();
            } else if (nNewPosY + this->m_pLogoPreview->height() >= this->m_rectVideoArea.bottom() + 2) {
                nNewPosY = this->m_rectVideoArea.bottom() - this->m_pLogoPreview->height() + 2;
            }

            this->m_pLogoPreview->move(nNewPosX, nNewPosY);

            this->m_nLogoX = nNewPosX - this->m_rectVideoArea.left();
            this->m_nLogoY = nNewPosY - this->m_rectVideoArea.top();
        }

        //*///////////////////////////////////////
        this->m_ptPrev = ptCur;
    }
}

void QUIMain::on_btnCaptionSave_clicked()
{
    this->updateCaptionList();
    int nLen = this->m_lstCaption.length();
    if (nLen == 0) return;

    QString strFile = QFileDialog::getSaveFileName(this, tr("Save Caption List"),
                                                   "captions.cap",
                                                   tr("Caption List File (*.cap)"));
    if (strFile.isEmpty()) return;

    QString strAll;

    for (int nId = 0 ; nId < nLen ; nId ++) {
        QString strRow = this->m_lstCaption.at(nId).timeStart.toString("hh:mm:ss.zzz") + "###" +
                         this->m_lstCaption.at(nId).timeEnd.toString("hh:mm:ss.zzz") + "###" +
                         this->m_lstCaption.at(nId).strCaption + "###" +
                         this->m_lstCaption.at(nId).strImage;
        strAll += strRow + "%%%";
    }

    strAll = strAll.left(strAll.length() - 3);

    QFile file(strFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << strAll;
    file.close();
}

void QUIMain::on_btnCaptionLoad_clicked()
{
    QString strFile = QFileDialog::getOpenFileName(this, tr("Open Caption List File"),
                                                   "./",
                                                   tr("Caption List File(*.cap)"));

    if (strFile.isEmpty()) return;

    QFile file(strFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    this->ui->tblCaption->disconnect(SIGNAL(cellChanged(int, int)));

    this->ui->tblCaption->clearContents();
    this->ui->tblCaption->setRowCount(0);

    QByteArray arrayCaption = file.readAll();
    QString strCaption(arrayCaption);
    QStringList strlstCaption = strCaption.split("%%%");
    int nLen = strlstCaption.length();
    for (int nId = 0 ; nId < nLen ; nId ++) {
        QString strRow = strlstCaption.at(nId);
        QStringList strlstRow = strRow.split("###");
        QString strTimeStart = strlstRow.at(0);
        QString strTimeEnd = strlstRow.at(1);
        QString strCaption = strlstRow.at(2);
        QString strImage = strlstRow.at(3);

        QTime timeStart = QTime::fromString(strTimeStart, "hh:mm:ss.zzz");
        QTime timeEnd = QTime::fromString(strTimeEnd, "hh:mm:ss.zzz");

        this->ui->tblCaption->setRowCount(nId + 1);
        this->ui->tblCaption->setRowHeight(nId, 45);

        // add caption to list
        QTableWidgetItem *pItemCheck = new QTableWidgetItem();
        pItemCheck->setCheckState(Qt::Unchecked);
        this->ui->tblCaption->setItem(nId, 0, pItemCheck);

        QString strStyleSheet = "selection-color: rgb(30, 30, 30);"
                                "selection-background-color: rgb(240, 240, 240);"
                                "color: rgb(255, 255, 255);";

        QTimeEdit *ptimeedtStart = new QTimeEdit(this->ui->tblCaption);
        ptimeedtStart->setStyleSheet(strStyleSheet);
        ptimeedtStart->setDisplayFormat("HH:mm:ss.zzz");
        ptimeedtStart->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ptimeedtStart->setTime(timeStart);
        this->ui->tblCaption->setCellWidget(nId, 1, ptimeedtStart);
        this->connect(ptimeedtStart, SIGNAL(timeChanged(QTime)), this, SLOT(us_updatePreview()));

        QTimeEdit *ptimeedtEnd = new QTimeEdit(this->ui->tblCaption);
        ptimeedtEnd->setStyleSheet(strStyleSheet);
        ptimeedtEnd->setDisplayFormat("HH:mm:ss.zzz");
        ptimeedtEnd->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ptimeedtEnd->setTime(timeEnd);
        this->ui->tblCaption->setCellWidget(nId, 2, ptimeedtEnd);
        this->connect(ptimeedtEnd, SIGNAL(timeChanged(QTime)), this, SLOT(us_updatePreview()));

        QTextEdit *ptextedtCaption = new QTextEdit(this->ui->tblCaption);
        ptextedtCaption->setStyleSheet(strStyleSheet);
        ptextedtCaption->setText(strCaption);
        this->ui->tblCaption->setCellWidget(nId, 3, ptextedtCaption);
        this->connect(ptextedtCaption, SIGNAL(textChanged()), this, SLOT(us_updatePreview()));

        QItemCB *pItemCB = new QItemCB(this->ui->tblCaption);
        pItemCB->setValue(strImage);
        this->ui->tblCaption->setCellWidget(nId, 4, pItemCB);
        this->connect(pItemCB, SIGNAL(valueChanged(QString)), this, SLOT(us_updatePreview()));
    }

    this->updateCaptionTime();
    this->updateCaptionList();

    this->connect(this->ui->tblCaption, SIGNAL(cellChanged(int, int)),
                  SLOT(us_tbl_cellChanged_Caption(int, int)));
}

void QUIMain::on_btnCaptionFont_clicked()
{
    bool bOk = false;
    QFont fontCaption = QFontDialog::getFont(&bOk, this->m_fontCaption, nullptr, "Select Caption Font");
    if (!bOk) return;

    this->m_fontCaption = fontCaption;

    QFont fontNew = fontCaption;
    fontNew.setPointSize(16);
    this->ui->lblCaptionFont->setFont(fontNew);
    this->us_updatePreview();
}

void QUIMain::initVideo(Video *pVideo)
{
    pVideo->bAvailable = false;
    pVideo->strFile = "";
    pVideo->strTitle = "";
    QPixmap pixTemp;
    pVideo->pixPreview = pixTemp;
    pVideo->nFrameCount = 0;
    pVideo->fFPS = 0.0;
    pVideo->nFourCC = 0;
    pVideo->nWidth = 0;
    pVideo->nHeight = 0;
    pVideo->time.setHMS(0, 0, 0, 0);
}

void QUIMain::setEnabledUIPlay(bool bEnabled)
{
    this->ui->btnRender->setEnabled(bEnabled);
    this->setEnabledUI(bEnabled);
}

void QUIMain::setEnabledUIRender(bool bEnabled)
{
    this->ui->btnPlay->setEnabled(bEnabled);
    this->ui->sldTime->setEnabled(bEnabled);
    this->setEnabledUI(bEnabled);
}

void QUIMain::setEnabledUI(bool bEnabled)
{
    this->ui->btnBatchAdd->setEnabled(bEnabled);
    this->ui->btnBatchRemove->setEnabled(bEnabled);
    this->ui->btnBatchClear->setEnabled(bEnabled);
    this->ui->tblBatch->setEnabled(bEnabled);
    this->ui->btnIntroLoad->setEnabled(bEnabled);
    this->ui->btnIntroClear->setEnabled(bEnabled);
    this->ui->btnLogoLoad->setEnabled(bEnabled);
    this->ui->btnLogoClear->setEnabled(bEnabled);
    this->ui->btnOutroLoad->setEnabled(bEnabled);
    this->ui->btnOutroClear->setEnabled(bEnabled);
    this->ui->btnCaptionAdd->setEnabled(bEnabled);
    this->ui->btnCaptionRemove->setEnabled(bEnabled);
    this->ui->btnCaptionClear->setEnabled(bEnabled);
    this->ui->tblCaption->setEnabled(bEnabled);
    this->ui->btnCaptionSave->setEnabled(bEnabled);
    this->ui->btnCaptionLoad->setEnabled(bEnabled);
    this->ui->btnCaptionFont->setEnabled(bEnabled);
}

bool QUIMain::canPlay()
{
    if (this->ui->tblBatch->rowCount() == 0) return false;
    if (this->ui->tblBatch->currentRow() == -1) return false;
    if (!this->m_videoIntro.bAvailable && this->m_pixLogo.width() == 0 && !this->m_videoOutro.bAvailable) return false;
    return true;
}
