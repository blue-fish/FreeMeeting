#include "dialog.h"

#include <QApplication>
#include"common.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //输出opencv  版本
    qDebug()<<"opencv version"<<cv::getVersionString().c_str();

    Dialog w;
    w.show();
    return a.exec();
}
