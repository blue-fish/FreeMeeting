HEADERS += \
    $$PWD/common.h \
    $$PWD/myfacedetact.h \
    $$PWD/screenread.h \
    $$PWD/theradworker.h \
    $$PWD/videoread.h

SOURCES += \
    $$PWD/myfacedetact.cpp \
    $$PWD/screenread.cpp \
    $$PWD/theradworker.cpp \
    $$PWD/videoread.cpp

INCLUDEPATH+=E:/colin/Qt/opencv/opencv-release/include/opencv2\
     E:/colin/Qt/opencv/opencv-release/include/

LIBS+=E:/colin/Qt/opencv/opencv-release/lib/libopencv_calib3d420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_core420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_features2d420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_flann420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_highgui420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_imgproc420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_ml420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_objdetect420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_video420.dll.a\
 E:\colin\Qt\opencv\opencv-release\lib\libopencv_videoio420.dll.a

## 使用openCV 4.5.4 开源 libfacedectection
#INCLUDEPATH += $$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/include\
#            $$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/include/opencv2

#LIBS+=$$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/lib/libopencv_*.dll.a


#INCLUDEPATH += $$PWD/libfacedetection/include

#LIBS+=$$PWD/libfacedetection/lib/libfacedetection.dll.a
