#include "qthreadffmpeg.h"
#include "common.h"


QThreadFFMpeg::QThreadFFMpeg(Stage stage) : QThread (nullptr)
{
    this->m_stage = stage;
    this->m_bStop = false;
    this->m_exitStatus = QProcess::NormalExit;
    this->m_processError = QProcess::UnknownError;
    this->connect(this, SIGNAL(finished()), this, SLOT(on_finished()));
}

void QThreadFFMpeg::run()
{
    emit this->stagePercentChanged(this->m_stage.strTitleProcessing, 0);
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");

    this->m_pProcess = new QProcess();

    this->m_pProcess->setProcessChannelMode(QProcess::MergedChannels);
    this->connect(this->m_pProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(on_process_finished(int, QProcess::ExitStatus)));
    this->connect(this->m_pProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(on_readyReadStdOutput()));

    this->m_pProcess->setProgram("ffmpeg.exe");
    this->m_pProcess->setArguments(this->m_stage.strlstArguments);
    this->m_pProcess->start();

//    qDebug("thread ffmpeg - thread started.");

    this->exec();
}

void QThreadFFMpeg::stop()
{
    this->m_bStop = true;

    if (this->m_pProcess != nullptr && this->m_pProcess->state() == QProcess::Running) {
        this->m_pProcess->terminate();

        int nCount = 0;
        while (this->m_pProcess->state() == QProcess::Running) {
            this->msleep(100);
            nCount ++;
            if (nCount > 100) break;
        }

        if (this->m_pProcess->state() == QProcess::Running) {
            this->m_pProcess->kill();

            int nCount = 0;
            while (this->m_pProcess->state() == QProcess::Running) {
                this->msleep(100);
                nCount ++;
                if (nCount > 100) break;
            }
        }
    }
}

void QThreadFFMpeg::on_process_finished(int, QProcess::ExitStatus exitStatus)
{
    qDebug("thread ffmpeg - process finished.");

    this->m_exitStatus = exitStatus;
    this->m_processError = this->m_pProcess->error();
    this->msleep(500);

    this->terminate();
}

void QThreadFFMpeg::on_readyReadStdOutput()
{
    this->m_pProcess->disconnect(SIGNAL(readyReadStandardOutput()));

    try {
        QByteArray aryOut = this->m_pProcess->readAllStandardOutput();
        int nLenRead = aryOut.length();
        if (nLenRead > 150) {
            aryOut = aryOut.right(150);
        }

        QString strOut(aryOut);
        strOut = strOut.trimmed();

        if (!strOut.isNull() && !strOut.isEmpty()) {
            QStringList strlstOut = strOut.split("\n");
            QString strLastLine = strlstOut.at(strlstOut.length() - 1);

//            qDebug("thread-ffmpeg - last line - %s", strLastLine.toUtf8().data());
//             size=   37632kB time=00:53:49.42 bitrate=  95.5kbits/s speed=72.9x

            QStringList strlstLastLine = strLastLine.split(" ");
            int nLen = strlstLastLine.length();
            for (int nId = 0 ; nId < nLen ; nId ++) {
                QString strItem = strlstLastLine.at(nId);
                if (strItem.startsWith("time=")) {
                    QString strTimeCur = strItem.right(strItem.length() - 5);
                    QTime timeCur = QTime::fromString(strTimeCur, "hh:mm:ss.z");
                    int nMsecCur = abs(timeCur.msecsTo(QTime(0, 0, 0, 0)));
                    int nPercent = int(double(nMsecCur) * 100.0 / double(this->m_stage.nTotalMsec));
                    emit this->stagePercentChanged(this->m_stage.strTitleProcessing, nPercent);
                }
            }
        }
    } catch (std::exception &e) {
        qDebug("thread ffmpeg - exception - %s", e.what());
    }

    this->connect(this->m_pProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(on_readyReadStdOutput()));
}

void QThreadFFMpeg::on_finished()
{
    qDebug("thread ffmpeg - thread finished.");
    if (this->m_bStop) {
        emit this->stagePercentChanged(this->m_stage.strTitleStopped, 100);
        emit this->stageFinished(this->m_stage.nStageId, N_STAGE_RET_STOPPED);
    } else {
        if (this->m_exitStatus == QProcess::CrashExit ||
                this->m_processError == QProcess::FailedToStart ||
                this->m_processError == QProcess::Crashed ||
                this->m_processError == QProcess::Timedout ||
                this->m_processError == QProcess::WriteError ||
                this->m_processError == QProcess::ReadError) {
            emit this->stagePercentChanged(this->m_stage.strTitleError, 100);
            emit this->stageFinished(this->m_stage.nStageId, N_STAGE_RET_ERROR);
        } else {
            QFile fileOutput(this->m_stage.strOutput);
            if (fileOutput.exists() && fileOutput.size() > 0) {
                emit this->stagePercentChanged(this->m_stage.strTitleSuccess, 100);
                emit this->stageFinished(this->m_stage.nStageId, N_STAGE_RET_SUCCESS);
            } else {
                emit this->stagePercentChanged(this->m_stage.strTitleError, 100);
                emit this->stageFinished(this->m_stage.nStageId, N_STAGE_RET_ERROR);
            }
        }
    }
}
