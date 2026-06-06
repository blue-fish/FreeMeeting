#include "screenread.h"
#include"common.h"
#include"QTime"
ScreenRead::ScreenRead(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT( slot_getSreenFrame()));
}

ScreenRead::~ScreenRead()
{
    if(m_timer)
    {
        m_timer->stop();
        delete m_timer;
        m_timer=NULL;
    }
}

void ScreenRead::slot_getSreenFrame()
{
    //获取桌面对象
    QScreen *src = QApplication::primaryScreen();
    //获取当前桌面图片
     QPixmap map = src->grabWindow( QApplication::desktop()->winId() );
     QImage image = map.toImage();
   //  QImage image = map.toImage().convertToFormat(QImage::Format_RGB888);
     //缩放，改变尺寸
    // image = image.scaled( 1600, 900, Qt::KeepAspectRatio, Qt::SmoothTransformation );
     qint64 time = QDateTime::currentMSecsSinceEpoch();
     Q_EMIT SIG_getScreenFrame(image,time);

}

void ScreenRead::slot_openVideo()
{
    m_timer->start(1000/FRAME_RATE -10);
}

void ScreenRead::slot_closeVideo()
{
    m_timer->stop();
}
