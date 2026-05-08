#include "videoread.h"
#include <QMessageBox>
#include<QDebug>
#include"QPainter"

VideoRead::VideoRead(QObject *parent) : QObject(parent),m_tuer(":/images/tuer.png"),m_hat(":/images/hat.png")
{
    m_timer = new QTimer;
    m_pVideoWorker = QSharedPointer<VideoWorker>(new VideoWorker);
    m_pVideoWorker.data()->slot_setInfo(this);
    connect(m_timer,SIGNAL(timeout()),m_pVideoWorker.data(),SLOT(slot_dowork()));
    //需要xml加到 exe同级目录里面
    MyFaceDetact::FaceDetectInit();
    m_moji=0;
}

VideoRead::~VideoRead()
{
    if(m_timer)
    {
        m_timer->stop();
        delete m_timer;
        m_timer=NULL;
    }
}
//摄像头采集图像流程
void VideoRead::slot_getVideoFram()
{
    qDebug()<<"slot_getVideoFram:"<<QThread::currentThread();
    //从摄像头对象读取数据
    Mat frame; //存储图象的对象，但不是【用于显示和传输】的格式（RGB）
               //opencv采集的格式 BGR
    if( !cap.read(frame) )
    {
     return;
    }
    //获取图片后 识别出人脸的位置，返回位置对应的矩形框
    //将 opencv 采集的 BGR 的图片类型转化为 RGB24（24种色域，每个颜色8位 ） 的类型
    //加滤镜 萌拍
    //人脸识别失败，使用上一次的人脸矩形
    std::vector<Rect> faces;
    //存放上一次的矩形
    //m_vecLastFace=
    if(m_moji !=0)
        MyFaceDetact::detectAndDisplay(frame,faces);
    cvtColor(frame,frame,CV_BGR2RGB);

    //定义 QImage 对象, 用于发送数据以及图片显示
    QImage image ((unsigned const char*)frame.data,frame.cols,frame.rows,QImage::Format_RGB888);
    //将道具绘制到图片
    QImage tmpImg;
    switch (m_moji) {
        case moji_tuer:
            tmpImg = m_tuer;
            break;
        case moji_hat:
            tmpImg = m_hat;
            break;
    }
    if(faces.size()>0)
        m_vecLastFace=faces;
     //将道具绘制到图片
    if(m_moji == moji_tuer || m_moji == moji_hat)
    {
        //QPainter 使用
        QPainter paint(&image);
        //遍历所有人脸的矩形
        for(int i=0; i<m_vecLastFace.size();i++)
        {
            Rect rct  = m_vecLastFace[i];
            int x=rct.x+rct.width*0.5 - tmpImg.width()*0.5+20 ;
            int y=rct.y - tmpImg.height();
            QPoint p(x,y);
            paint.drawImage(p,tmpImg);

        }
    }
    //【缩放操作】转化为大小更小的图片 保持缩放比
    image = image.scaled( IMAGE_WIDTH,IMAGE_HEIGHT, Qt::KeepAspectRatio );
    //发送图片
    Q_EMIT SIG_sendVideoFrame( image );
}

void VideoRead::slot_openVideo()
{
    m_timer->start(1000/FRAME_RATE-10);//-10是为了修正定时器的滞后偏差
    //打开摄像头
    cap.open(0);//打开默认摄像头
    if(!cap.isOpened()){
     QMessageBox::information(NULL,tr("提示"),tr("视频没有打开"));
     return;
    }
}

void VideoRead::slot_closeVideo()
{
    m_timer->stop();
    //关闭摄像头
    m_timer->stop();
    if(cap.isOpened())
        cap.release();
}

void VideoRead::slot_setMoji(int newMoji)
{
     qDebug()<<__func__;
    m_moji = newMoji;
}
