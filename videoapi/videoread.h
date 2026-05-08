#ifndef VIDEOREAD_H
#define VIDEOREAD_H

#include <QObject>
#include<QImage>
#include<QTimer>
#include"common.h"
#include"theradworker.h"
#include"myfacedetact.h"

//定义压缩图片的宽高
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
    void slot_getVideoFram();
    void slot_openVideo();
    void slot_closeVideo();
    void slot_setMoji(int newMoji);
private:
    QTimer * m_timer;
    //opencv 获取图片对象
    cv::VideoCapture cap;
    QSharedPointer<VideoWorker> m_pVideoWorker;
    std::vector<Rect> m_vecLastFace;

    enum moji_type{moji_tuer=1,moji_hat };
    int m_moji;//存储萌拍效果 1.兔耳朵，2.帽子

    QImage m_tuer;
    QImage m_hat;
};

class VideoWorker:public ThreadWorker
{
    Q_OBJECT
public slots:
    void slot_setInfo(VideoRead*p)
    {
        m_pVideoRead = p;
    }
    //定时器到时，执行
    void slot_dowork()
    {

        m_pVideoRead->slot_getVideoFram();
    }
private:
    VideoRead* m_pVideoRead;

};
#endif // VIDEOREAD_H
