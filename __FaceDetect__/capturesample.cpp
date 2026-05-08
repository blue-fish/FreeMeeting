#include "capturesample.h"

CaptureSample::CaptureSample(QObject *parent) : QObject(parent)
{
    m_timer.setInterval(1000/25);

    connect(&m_timer,SIGNAL(timeout()),
            this,SLOT(slot_timerTimeOut()));

    m_faceDetect=new FaceDetect;
}

CaptureSample::~CaptureSample()
{
    delete m_faceDetect;
}

void CaptureSample::slot_openCapture()
{
    qDebug()<<__func__;
    if(!m_capture.isOpened())
        m_capture.open(0);//打开默认摄像头

    m_timer.start();

}

void CaptureSample::slot_closeCapture()
{
    qDebug()<<__func__;
    if(m_capture.isOpened())
        m_capture.release();//打开默认摄像头

    m_timer.stop();

}

void CaptureSample::slot_timerTimeOut()
{
    qDebug()<<__func__;

    //读取摄像头数据
    if(!m_capture.isOpened()){
        return;
    }
    Mat srcFrame;
    if(!m_capture.read(srcFrame)){
        qDebug()<<"摄像头读取数据失败";
        return;
    }

    //左右翻转
    flip(srcFrame,srcFrame,1);

    m_faceDetect->faceDetectAndDraw(srcFrame);

    //opencv bge 格式 转化为RGB888
    Mat frameRes;
    cvtColor(srcFrame,frameRes,CV_BGR2RGB);

    QImage img((unsigned const char*)frameRes.data,
               frameRes.cols,frameRes.rows,QImage::Format_RGB888);
    //发送信号
    QImage copy=img.copy();
    Q_EMIT SIG_getOneImage(copy);
}
