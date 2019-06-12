#ifndef QTHREADPLAY_H
#define QTHREADPLAY_H

#include <QThread>
#include "quimain.h"

class QThreadPlay : public QThread
{
    Q_OBJECT
public:
    explicit QThreadPlay(QUIMain *pMain, int nCurVideo);

signals:
    void playFrame(int nFrameSrc, QPixmap pixFrame, int nFrameNumber);

public slots:

private:
    bool m_bStop;
    QUIMain *m_pMain;
    int m_nCurVideo;

public:
    void run();
    void stop();
};

#endif // QTHREADPLAY_H
