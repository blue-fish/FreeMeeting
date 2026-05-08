#ifndef CAPTURESAMPLE_H
#define CAPTURESAMPLE_H

#include <QObject>
#include"common.h"
#include<QTimer>

#include"facedetect.h"

class CaptureSample : public QObject
{
    Q_OBJECT
public:
    explicit CaptureSample(QObject *parent = nullptr);

    ~CaptureSample();
signals:
    void SIG_getOneImage(QImage& img);
public slots:
    void slot_openCapture();

    void slot_closeCapture();

    void slot_timerTimeOut();

private:
    //opencv 摄像头
    cv::VideoCapture m_capture;
    //定时器
    QTimer m_timer;

    //人脸检测的类
    FaceDetect* m_faceDetect;
};

#endif // CAPTURESAMPLE_H
