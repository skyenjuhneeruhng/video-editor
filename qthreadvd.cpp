#include "qthreadvd.h"
#include "common.h"
#include "qcv.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

QThreadVD::QThreadVD(QUIMain *pMain, int nVideo, QString strOutput) : QThread (nullptr)
{
    this->m_pMain = pMain;
    this->m_nVideo = nVideo;
    this->m_strOutput = strOutput;
    this->m_bStop = false;
    this->m_bError = false;

    this->m_strStageTitleProcessing = "vd processing ...";
    this->m_strStageTitleSuccess = "vd processing has been finished successfully.";
    this->m_strStageTitleError = "vd processing has been finished.";
    this->m_strStageTitleStopped = "vd processing was stopped.";

    this->connect(this, SIGNAL(finished()), this, SLOT(on_finished()));
}

void QThreadVD::run()
{
    Video videoBatch = this->m_pMain->m_lstBatch.at(this->m_nVideo);
    emit this->stagePercentChanged(this->m_strStageTitleProcessing, 0);

    // open batch input video file
    cv::VideoCapture capVideo(videoBatch.strFile.toStdString());
    if (!capVideo.isOpened()) {
        qDebug("thread_vd_video - [%s] - video_capture - open error", videoBatch.strFile.toUtf8().data());
        this->m_bError = true;
        return;
    }

    // open batch output video file
    cv::VideoWriter capWriter;
    int nFourCC = cv::VideoWriter::fourcc('D', 'I', 'V', 'X');
    capWriter.open(this->m_strOutput.toStdString(), nFourCC, videoBatch.fFPS,
                   cv::Size(videoBatch.nWidth, videoBatch.nHeight));

    if (!capWriter.isOpened()) {
        qDebug("thread_vd_video - [%s] - video_writer - open error", this->m_strOutput.toUtf8().data());
        this->m_bError = true;
        return;
    }

    // update video area & logo preview
    this->m_pMain->updateVideoAreaLogo(this->m_nVideo);

    int nCount = 0;
    QTime timeCur = this->m_pMain->m_videoIntro.time;
    int nFrameNumber = 0;
    while (!this->m_bStop) {
        cv::Mat matFrame;
        if (!capVideo.read(matFrame)) {
            break;
        }
        QPixmap pixFrame = QCV::cvMatToQPixmap(matFrame);

        int nMsecBatch = int(double(nFrameNumber) / this->m_pMain->m_lstBatch.at(this->m_nVideo).fFPS * 1000.0);
        timeCur = this->m_pMain->m_videoIntro.time.addMSecs(nMsecBatch);

        // draw caption
        int nCaptionCount = this->m_pMain->m_lstCaption.length();
        for (int nId = 0 ; nId < nCaptionCount ; nId ++) {
            if (timeCur < this->m_pMain->m_lstCaption.at(nId).timeStart) continue;
            if (timeCur > this->m_pMain->m_lstCaption.at(nId).timeEnd) continue;

            // draw the first caption
            this->m_pMain->drawCaption(&pixFrame, this->m_pMain->m_lstCaption.at(nId).strCaption,
                                       this->m_pMain->m_lstCaption.at(nId).strImage);
            break;
        }

        // draw logo
        this->m_pMain->drawLogo(&pixFrame);

        matFrame = QCV::QPixmapToCvMat(pixFrame);
        capWriter.write(matFrame);
        nFrameNumber ++;
        nCount ++;
        if (nCount > 10) {
            nCount = 0;
            int nPercent = int(double(nFrameNumber * 100) / double(videoBatch.nFrameCount));
            emit this->stagePercentChanged(this->m_strStageTitleProcessing, nPercent);
        }
    }

    capVideo.release();
    capWriter.release();
}

void QThreadVD::stop()
{
    this->m_bStop = true;
}

void QThreadVD::on_finished()
{
    if (this->m_bError) {
        emit this->stagePercentChanged(this->m_strStageTitleError, 100);
        emit this->stageFinished(N_STAGE_ID_VD, N_STAGE_RET_ERROR);
    } else {
        if (this->m_bStop) {
            emit this->stagePercentChanged(this->m_strStageTitleStopped, 100);
            emit this->stageFinished(N_STAGE_ID_VD, N_STAGE_RET_STOPPED);
        } else {
            emit this->stagePercentChanged(this->m_strStageTitleSuccess, 100);
            emit this->stageFinished(N_STAGE_ID_VD, N_STAGE_RET_SUCCESS);
        }
    }
}
