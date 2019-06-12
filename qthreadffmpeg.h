#ifndef QTHREADFFMPEG_H
#define QTHREADFFMPEG_H

#include <QThread>
#include <QProcess>
#include "quimain.h"

class QThreadFFMpeg : public QThread
{
    Q_OBJECT
public:
    explicit QThreadFFMpeg(Stage stage);

signals:
    void stagePercentChanged(QString strStageTitle, int nPercent);
    void stageFinished(int nStageId, int nRet);

public slots:
    void on_process_finished(int exitCode, QProcess::ExitStatus exitStatus);
    void on_readyReadStdOutput();
    void on_finished();

private:
    Stage m_stage;
    QProcess *m_pProcess;
    bool m_bStop;

    QProcess::ExitStatus m_exitStatus;
    QProcess::ProcessError m_processError;

public:
    void run();
    void stop();
};

#endif // QTHREADFFMPEG_H
