#ifndef QUIMAIN_H
#define QUIMAIN_H

#include <QWidget>
#include <QCheckBox>
#include <QMouseEvent>
#include <QThread>
#include <QTime>
#include <QFont>
#include "qlogo.h"
#include "global.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

namespace Ui {
class QUIMain;
}

class QUIMain : public QWidget
{
    Q_OBJECT

public:
    explicit QUIMain(QWidget *parent = nullptr);
    ~QUIMain();

private slots:
    void on_btnBatchAdd_clicked();
    void us_chk_stateChanged_Batch(int nState);
    void us_tbl_cellChanged_Batch(int nRow, int nCol);
    void on_btnCaptionAdd_clicked();
    void us_chk_stateChanged_Caption(int nState);
    void us_tbl_cellChanged_Caption(int nRow, int nCol);
    void on_btnIntroLoad_clicked();
    void on_btnIntroClear_clicked();
    void on_btnLogoLoad_clicked();
    void on_btnLogoClear_clicked();
    void on_btnOutroLoad_clicked();
    void on_btnOutroClear_clicked();
    void on_btnBatchRemove_clicked();
    void on_btnBatchClear_clicked();
    void on_btnCaptionRemove_clicked();
    void on_btnCaptionClear_clicked();
    void on_tblBatch_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_splitterMoved(int pos, int index);
    void on_sldTime_valueChanged(int value);
    void on_btnPlay_clicked();
    void on_playFrame(int nFrameSrc, QPixmap pixFrame, int nFrameNumber);
    void us_threadPlay_finished();
    void on_btnRender_clicked();
    void on_stagePercentChanged(QString strStageTitle, int nPercent);
    void on_stageFinished(int nStageId, int nRet);
    void us_logo_mouseMove();
    void on_btnCaptionSave_clicked();
    void on_btnCaptionLoad_clicked();
    void on_btnCaptionFont_clicked();
    void us_updatePreview();

private:
    Ui::QUIMain *ui;

public:
    QCheckBox *m_pchkBatch;
    QCheckBox *m_pchkCaption;
    QLogo *m_pLogoPreview;

    QFont m_fontCaption; // font for caption string

    QList<Video> m_lstBatch; // batch video info list
    Video m_videoIntro; // intro video info
    Video m_videoOutro; // outro video info
    QString m_strLogoFile; // logo file path
    QPixmap m_pixLogo; // logo image - pixmap

    bool m_bIsLogoMoving; // logo preview image position changing & moving state
    QPoint m_ptPrev; // logo preview - preview position
    int m_nLogoX; // logo preview position in video area - x
    int m_nLogoY; // logo preview position in video area - y
    int m_nLogoWidth; // logo preview width
    int m_nLogoHeight; // logo preview height
    int m_nLogoMouse; // logo mouse cursor
    double m_fLogoRatio; // logo image ratio

    QRect m_rectVideoArea; // preview - video area

    QTime m_timeTotal; // total time - intro + video + outro
    int m_nMsecTotal; // total time(milisecond) - intro + video + outro

    QTime m_timeMid; // middle time - intro + video

    QTime m_timeCur; // current time which slider shows
    int m_nMsecCur; // current time(milisecond) which slider shows

    bool m_bIsPlaying; // playing preview
    bool m_bIsRendering; // rendering
    QList<Caption> m_lstCaption; // caption list

    int m_nCurRendering; // rendering - current batch video id
    double m_fTotalProgStep; // rendering - total progress - increase step
    QString m_strCurFPS; // rendering - current video file FPS
    QString m_strCurScale; // rendering - current video resolution
    QString m_strFolderTmp; // rendering - temp folder
    QString m_strFolderOut; // rendering - output folder
    QThread::Priority m_ThreadPriority; // rendering - thread priority

public:
    void updateVideoAreaLogo(int nVideo);
    void drawCaption(QPixmap *ppixFrame, QString strCaption, QString strImage);
    void drawLogo(QPixmap *ppixFrame);

private:
    void init();
    void initPreviewVideo();
    void initPreviewPanel();
    void initVideo(Video *pVideo);
    bool previewVideo(QString strFile, Video *pVideo);
    void updateUI();
    void updateCaptionTime();
    void updateCaptionList();
    QTime calcTime(double fFPS, int nFrameCount);
    bool grabFrame(Video *pVideo, QTime *pTimeCur, QPixmap *ppixFrame);
    QTime timeDiff(QTime *pTime1, QTime *pTime2);
    QTime timeAdd(QTime *pTime1, QTime *pTime2);
    QString addFileSuffix(QString strFolder, QString strFile, QString strSuffix);
    void restoreFromRendering();
    void setEnabledUIPlay(bool bEnabled);
    void setEnabledUIRender(bool bEnabled);
    void setEnabledUI(bool bEnabled);
    bool canPlay();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
};

#endif // QUIMAIN_H
