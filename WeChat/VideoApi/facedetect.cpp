#include "facedetect.h"

FaceDetect::FaceDetect(QObject *parent) : QObject(parent)
{
    m_pBuffer = (unsigned char *)malloc(DETECT_BUFFER_SIZE);
}

FaceDetect::~FaceDetect()
{
    free(m_pBuffer);
}

void FaceDetect::faceDetectAndDraw(Mat &frame, std::vector<Rect> &vec)
{

    Mat& image = frame;

    ///////////////////////////////////////////
    // CNN face detection
    // Best detection rate
    //////////////////////////////////////////
    //!!! The input image must be a BGR one (three-channel) instead of RGB
    //!!! DO NOT RELEASE pResults !!!


    int *pResults = facedetect_cnn(m_pBuffer, (unsigned char*)(image.ptr(0)), image.cols, image.rows, (int)image.step);


    Mat& result_image = image;
    //print the detection results
    for(int i = 0; i < (pResults ? *pResults : 0); i++)
    {
        short * p = ((short*)(pResults+1)) + 16*i;
        //人脸检测结果  人脸的矩形区域
        int confidence = p[0];//可信度
        int x = p[1];
        int y = p[2];
        int w = p[3];
        int h = p[4];

        if(confidence>=60){//过滤可能错误的
            //draw face rectangle
            //rectangle(result_image, Rect(x, y, w, h), Scalar(0, 255, 0), 2);
            vec.emplace_back(Rect(x, y, w, h));
        }
    }

}
