#include "quimain.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QUIMain w;
    w.showMaximized();

    return app.exec();
}
