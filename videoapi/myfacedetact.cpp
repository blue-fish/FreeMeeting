#include "myfacedetact.h"

//用于做人脸识别的对象，加载xml，调用方法
//CascadeClassifier face_cascade;
//CascadeClassifier eyes_cascade;

#define DETECT_BUFFER_SIZE 0x20000
static unsigned char* g_pResultBuffer = nullptr;

MyFaceDetact::MyFaceDetact(QObject *parent) : QObject(parent)
{

}

void MyFaceDetact::FaceDetectInit()
{
    // libfacedetection CNN 模型初始化
    // 分配检测结果缓冲区
    if (!g_pResultBuffer)
    {
        g_pResultBuffer = new unsigned char[DETECT_BUFFER_SIZE];
    }
    qDebug() << "FaceDetectInit: libfacedetection CNN ready, buffer:" << static_cast<void*>(g_pResultBuffer);
}

void MyFaceDetact::detectAndDisplay(Mat &frame, std::vector<Rect> &faces)
{
    static int s_frameCount = 0;
    ++s_frameCount;
    faces.clear();

    if (!g_pResultBuffer) {
        qDebug() << "FaceDetect: result buffer is null";
        return;
    }

    if (frame.empty() || frame.channels() != 3) {
        qDebug() << "FaceDetect: invalid frame, empty:" << frame.empty() << "channels:" << frame.channels();
        return;
    }

    // 使用 libfacedetection CNN 模型检测人脸
    // facedetect_cnn 要求输入 BGR 格式图像
    int* pResults = facedetect_cnn(
        g_pResultBuffer,
        frame.ptr(0),         // BGR 图像数据
        frame.cols,           // 宽度
        frame.rows,           // 高度
        (int)frame.step       // 步长
    );

    if (!pResults) {
        if (s_frameCount % 30 == 0) {
            qDebug() << "FaceDetect: facedetect_cnn returned null";
        }
        return;
    }

    int nFace = *pResults;
    for (int i = 0; i < nFace; i++)
    {
        // libfacedetection returns 16 short values per face:
        // confidence, x, y, w, h, landmarks...
        short* p = ((short*)(pResults + 1)) + 16 * i;
        int confidence = p[0];
        int x = p[1];
        int y = p[2];
        int w = p[3];
        int h = p[4];

        // 跳过置信度过低的人脸
        if (confidence < 60 || w <= 0 || h <= 0) continue;

        faces.push_back(Rect(x, y, w, h));
    }

    if (s_frameCount % 30 == 0 || !faces.empty()) {
        qDebug() << "FaceDetect: raw faces:" << nFace
                 << "accepted:" << faces.size()
                 << "frame:" << frame.cols << "x" << frame.rows
                 << "step:" << frame.step;
    }

    //在原图上绘制人脸椭圆(保留原有绘制逻辑)
    for (auto ite = faces.begin(); ite != faces.end(); ++ite)
    {
        Rect rct = *ite;
        Point center( rct.x + rct.width*0.5, rct.y + rct.height*0.5 );
        ellipse( frame, center, Size( rct.width*0.5, rct.height*0.5), 0, 0, 360,
                 Scalar( 255, 0, 255 ), 4, 8, 0 );
    }
}
