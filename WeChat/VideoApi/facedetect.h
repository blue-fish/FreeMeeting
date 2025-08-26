#ifndef FACEDETECT_H
#define FACEDETECT_H

#include <QObject>

#include<common.h>

//人脸检测缓冲区
//define the buffer size. Do not change the size!
//0x9000 = 1024 * (16 * 2 + 4), detect 1024 face at most
#define DETECT_BUFFER_SIZE 0x9000

class FaceDetect : public QObject
{
    Q_OBJECT
public:
    explicit FaceDetect(QObject *parent = nullptr);
    ~FaceDetect();

    //接收一个图片，然后在图片上面检测人脸  并且绘制人脸区域
    void faceDetectAndDraw(Mat& frame,std::vector<Rect>& vec);
signals:

private:
    unsigned char * m_pBuffer ;

};

#endif // FACEDETECT_H
