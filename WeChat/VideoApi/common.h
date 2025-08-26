#ifndef COMMON_H
#define COMMON_H

// 在原有基础上添加
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}


#include<QObject>

#define FRAME_RATE (25)

#include<opencv2/imgproc/types_c.h>
#include<opencv2/opencv.hpp>
#include<opencv2/face.hpp>
#include<highgui/highgui.hpp>
#include<imgproc/imgproc.hpp>
#include<core/core.hpp>

using namespace cv;

#include<QDebug>
#include<QImage>

//加人脸加测的头文件
#include<opencv2/opencv.hpp>
#include"facedetectcnn.h"

#endif // COMMON_H
