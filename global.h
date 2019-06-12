#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QPixmap>
#include <QTime>

struct Video {
    bool bAvailable;
    QString strFile;
    QString strTitle;
    QPixmap pixPreview;
    int nFrameCount;
    double fFPS;
    int nFourCC;
    int nWidth;
    int nHeight;
    QTime time;
};

struct Caption {
    QTime timeStart;
    QTime timeEnd;
    QString strCaption;
    QString strImage;
};

struct Stage {
    int nStageId;
    QStringList strlstArguments;
    int nTotalMsec;
    QString strTitleProcessing;
    QString strTitleSuccess;
    QString strTitleError;
    QString strTitleStopped;
    QString strOutput;
};

#endif // GLOBAL_H
