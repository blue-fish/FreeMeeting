#include "ckernel.h"

#include <QApplication>

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    WeChatDialog w;
//    w.show();
      Ckernel::GetInstance();
    return a.exec();
}
