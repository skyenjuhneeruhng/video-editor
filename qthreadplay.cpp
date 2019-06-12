#include "qthreadplay.h"
#include "common.h"
#include "qcv.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

QThreadPlay::QThreadPlay(QUIMain *pMain, int nCurVideo) : QThread(nullptr)
{
    this->m_bStop = false;
    this->m_pMain = pMain;
    this->m_nCurVideo = nCurVideo;
}

void QThreadPlay::run()
{
    bool bPlayedBefore = false;
    // check if current time line in intro video
    if (this->m_pMain->m_timeCur <= this->m_pMain->m_videoIntro.time) {
        cv::VideoCapture capVideo(this->m_pMain->m_videoIntro.strFile.toStdString());
        if (!capVideo.isOpened()) {
            qDebug("thread_play_video - intro - [%s] - video_capture - open error",
                   this->m_pMain->m_videoIntro.strFile.toUtf8().data());
            this->m_pMain->m_videoIntro.bAvailable = false;
            return;
        }

        // calc frame number
        int nMsec = abs(this->m_pMain->m_timeCur.msecsTo(QTime(0, 0, 0, 0)));
        int nFrameNumber = int(double(nMsec) * this->m_pMain->m_videoIntro.fFPS / 1000.0);
        capVideo.set(cv::CAP_PROP_POS_FRAMES, nFrameNumber);
        bPlayedBefore = true;

        unsigned long nDelay = static_cast<unsigned long>(1000.0 / this->m_pMain->m_videoIntro.fFPS);

        while (!this->m_bStop) {
            cv::Mat matFrame;
            if (!capVideo.read(matFrame)) {
                break;
            }

            QPixmap pixFrame = QCV::cvMatToQPixmap(matFrame);
            emit this->playFrame(N_FRAME_SRC_INTRO, pixFrame, nFrameNumber);
            this->msleep(nDelay);

            nFrameNumber ++;
        }

        capVideo.release();
    }

    if (this->m_bStop) return;

    // check if current time line in batch video
    if (bPlayedBefore || (this->m_pMain->m_timeCur > this->m_pMain->m_videoIntro.time
                          && this->m_pMain->m_timeCur <= this->m_pMain->m_timeMid)) {
        cv::VideoCapture capVideo(this->m_pMain->m_lstBatch.at(this->m_nCurVideo).strFile.toStdString());
        if (!capVideo.isOpened()) {
            qDebug("thread_play_video - batch - [%s] - video_capture - open error",
                   this->m_pMain->m_lstBatch.at(this->m_nCurVideo).strFile.toUtf8().data());
            return;
        }

        // calc frame number
        int nFrameNumber = 0;
        if (!bPlayedBefore) {
            int nMsec = abs(this->m_pMain->m_timeCur.msecsTo(this->m_pMain->m_videoIntro.time));
            nFrameNumber = int(double(nMsec) * this->m_pMain->m_lstBatch.at(this->m_nCurVideo).fFPS / 1000.0);
            capVideo.set(cv::CAP_PROP_POS_FRAMES, nFrameNumber);
            bPlayedBefore = true;
        }

        unsigned long nDelay = static_cast<unsigned long>(1000.0 / this->m_pMain->m_lstBatch.at(this->m_nCurVideo).fFPS);

        while (!this->m_bStop) {
            cv::Mat matFrame;
            if (!capVideo.read(matFrame)) {
                break;
            }

            QPixmap pixFrame = QCV::cvMatToQPixmap(matFrame);
            emit this->playFrame(N_FRAME_SRC_BATCH, pixFrame, nFrameNumber);
            this->msleep(nDelay);

            nFrameNumber ++;
        }

        capVideo.release();
    }

    if (this->m_bStop) return;

    // check if current time line in outro video
    if (bPlayedBefore || (this->m_pMain->m_timeCur > this->m_pMain->m_timeMid)) {
        cv::VideoCapture capVideo(this->m_pMain->m_videoOutro.strFile.toStdString());
        if (!capVideo.isOpened()) {
            qDebug("thread_play_video - outro - [%s] - video_capture - open error",
                   this->m_pMain->m_videoOutro.strFile.toUtf8().data());
            this->m_pMain->m_videoOutro.bAvailable = false;
            return;
        }

        // calc frame number
        int nFrameNumber = 0;
        if (!bPlayedBefore) {
            int nMsec = abs(this->m_pMain->m_timeCur.msecsTo(this->m_pMain->m_timeMid));
            nFrameNumber = int(double(nMsec) * this->m_pMain->m_videoOutro.fFPS / 1000.0);
            capVideo.set(cv::CAP_PROP_POS_FRAMES, nFrameNumber);
        }

        unsigned long nDelay = static_cast<unsigned long>(1000.0 / this->m_pMain->m_videoOutro.fFPS);

        while (!this->m_bStop) {
            cv::Mat matFrame;
            if (!capVideo.read(matFrame)) {
                break;
            }

            QPixmap pixFrame = QCV::cvMatToQPixmap(matFrame);
            emit this->playFrame(N_FRAME_SRC_OUTRO, pixFrame, nFrameNumber);
            this->msleep(nDelay);

            nFrameNumber ++;
        }

        capVideo.release();
    }

    qDebug("thread play finished");
}

void QThreadPlay::stop()
{
    this->m_bStop = true;
}
