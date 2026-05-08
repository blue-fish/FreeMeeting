#ifndef MYFACEDETACT_H
#define MYFACEDETACT_H

#include <QObject>
#include"common.h"

class MyFaceDetact : public QObject
{
    Q_OBJECT
public:
    explicit MyFaceDetact(QObject *parent = nullptr);

signals:

public slots:
    // 人脸识别的初始化
    static void FaceDetectInit();
    //获取摄像头图片后，识别人脸位置，返回位置对应的矩形框
    static void detectAndDisplay(Mat &frame,std::vector<Rect>&faces);


};

#endif // MYFACEDETACT_H
