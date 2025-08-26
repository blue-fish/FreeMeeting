#include "ckernel.h"

#include <QApplication>
#include <QMetaType>

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug() << "Failed to initialize SDL: " << SDL_GetError() ;
        return 1;
    }
//    WeChatDialog w;
//    w.show();
    Ckernel::GetInstance();

    return a.exec();
}
