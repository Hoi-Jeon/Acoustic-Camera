#include "mainwindow.h"
#include <QApplication>

#include "qcustomplot.h"

int main(int argc, char *argv[])
{
    QApplication SoundCamera(argc, argv);
    MainWindow w;
    w.show();

    return SoundCamera.exec();
}
