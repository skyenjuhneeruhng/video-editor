#ifndef QTHREADVD_H
#define QTHREADVD_H

#include <QThread>
#include "quimain.h"

class QThreadVD : public QThread
{
    Q_OBJECT
public:
    explicit QThreadVD(QUIMain *pMain, int nVideo, QString strOutput);

signals:
    void stagePercentChanged(QString strStageTitle, int nPercent);
    void stageFinished(int nStageId, int nRet);

public slots:
    void on_finished();

private:
    QUIMain *m_pMain;
    int m_nVideo;
    bool m_bStop;
    bool m_bError;
    QString m_strOutput;
    QString m_strStageTitleProcessing;
    QString m_strStageTitleSuccess;
    QString m_strStageTitleError;
    QString m_strStageTitleStopped;

public:
    void run();
    void stop();
};

#endif // QTHREADVD_H
