#ifndef VIDEOREAD_H
#define VIDEOREAD_H

#include <QObject>
#include<QImage>
#include <QTimer>
#include"common.h"
#include"myfacedetect.h"
#include"threadworker.h"
#include"facedetect.h"

//图片的宽高
#define IMAGE_WIDTH (320)
#define IMAGE_HEIGHT (240)

class VideoWorker;


class VideoRead : public QObject
{
    Q_OBJECT
public:
    explicit VideoRead(QObject *parent = nullptr);
    ~VideoRead();


signals:
    void SIG_sendVideoFrame(QImage img);
public slots:
    void slot_getVideoFrame();
    void slot_openVideo();
    void slot_closeVideo();
    void slot_setMoji(int newMoji);
private:
    QTimer *m_timer;

    //opencv获取图片对象
    cv::VideoCapture cap;

    QSharedPointer<VideoWorker> m_pVideoWorker;

    std::vector<Rect> m_vecLastFace;

    FaceDetect* m_faceDetect;

    enum moji_type{moji_tuer=1,moji_hat};
    int m_moji;//用于存储当前的效果 1兔耳朵 2帽子
    QImage m_tuer;
    QImage m_hat;
};

class VideoWorker:public ThreadWorker
{
    Q_OBJECT
public slots:
    void slot_setInfo(VideoRead* p){
        m_pVideoRead=p;
    }
    //定时器到时，执行
    void slot_doWork(){
        m_pVideoRead->slot_getVideoFrame();
    }
private:
    VideoRead* m_pVideoRead;


};
#endif // VIDEOREAD_H
