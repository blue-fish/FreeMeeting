HEADERS += \
    $$PWD/common.h \
    $$PWD/facedetect.h \
    $$PWD/myfacedetect.h \
    $$PWD/screenread.h \
    $$PWD/threadworker.h \
    $$PWD/videoread.h

SOURCES += \
    $$PWD/facedetect.cpp \
    $$PWD/myfacedetect.cpp \
    $$PWD/screenread.cpp \
    $$PWD/threadworker.cpp \
    $$PWD/videoread.cpp

#INCLUDEPATH+=C:/Qt/opencv-release/include/opencv2\
#            C:/Qt/opencv-release/include

#LIBS+=C:\Qt\opencv-release\lib\libopencv_calib3d420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_core420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_features2d420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_flann420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_highgui420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_imgproc420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_ml420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_objdetect420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_video420.dll.a\
#         C:\Qt\opencv-release\lib\libopencv_videoio420.dll.a

#使用 opencv 4.5.4 开源 libfacedetection
#三步 ： 添加头文件  添加动态库引入库 添加动态库
INCLUDEPATH += $$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/include\
            $$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/include/opencv2

LIBS+=$$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/lib/libopencv_*.dll.a


INCLUDEPATH += $$PWD/libfacedetection/include

LIBS+=$$PWD/libfacedetection/lib/libfacedetection.dll.a
