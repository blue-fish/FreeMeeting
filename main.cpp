#include "ckernel.h"
#include <QApplication>
#include <QMessageBox>

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //QMessageBox::information(nullptr, "Debug", "QApplication created");
    Ckernel::GetInstance();
    //QMessageBox::information(nullptr, "Debug", "Kernel created");
    return a.exec();
}
