#ifndef COMMON_H
#define COMMON_H
#include"highgui/highgui.hpp"
#include"imgproc/imgproc.hpp"
#include"core/core.hpp"
#include"opencv2\imgproc\types_c.h"


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}
using namespace cv;

#include<QDebug>
#include<QImage>


//#include<opencv2/imgproc/types_c.h>
////#include<opencv2/opencv.hpp>
//#include<opencv2/face.hpp>
//#include<highgui/highgui.hpp>
//#include<imgproc/imgproc.hpp>
//#include<core/core.hpp>

using namespace cv;

#include<QDebug>
#include<QImage>

//#include<opencv2/opencv.hpp>
#include"facedetectcnn.h"


#define FRAME_RATE (25)
#endif // COMMON_H
