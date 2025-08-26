QT       += core gui network
QT+= multimedia


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include(./netapi/netapi.pri)
INCLUDEPATH += ./netapi

include(./md5/md5.pri)
INCLUDEPATH += ./md5

#include(./AudioApi/audioapi.pri)
#INCLUDEPATH += ./AudioApi

include(./VideoApi/videoapi.pri)
INCLUDEPATH += ./VideoApi

include(./OpusApi/OpusApi.pri)
INCLUDEPATH += ./OpusApi/

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    avsyncmanager.cpp \
    ckernel.cpp \
    deepseek.cpp \
    logindialog.cpp \
    main.cpp \
    roomdialog.cpp \
    usershow.cpp \
    videodecoder.cpp \
    videoencoder.cpp \
    wechatdialog.cpp

HEADERS += \
    avsyncmanager.h \
    ckernel.h \
    deepseek.h \
    logindialog.h \
    roomdialog.h \
    usershow.h \
    videodecoder.h \
    videoencoder.h \
    wechatdialog.h

FORMS += \
    deepseek.ui \
    logindialog.ui \
    roomdialog.ui \
    usershow.ui \
    wechatdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc



#INCLUDEPATH += $$PWD/ffmpeg-4.2.2/include\
#     C:/Qt/opencv-release/include/opencv2\
#     C:/Qt/opencv-release/include

INCLUDEPATH += $$PWD/ffmpeg-4.2.2/include\
     E:/colin/Qt/opencv/opencv-release/include/opencv2\
     E:/colin/Qt/opencv/opencv-release/include

LIBS += $$PWD/ffmpeg-4.2.2/lib/avcodec.lib\
         $$PWD/ffmpeg-4.2.2/lib/avdevice.lib\
         $$PWD/ffmpeg-4.2.2/lib/avfilter.lib\
         $$PWD/ffmpeg-4.2.2/lib/avformat.lib\
         $$PWD/ffmpeg-4.2.2/lib/avutil.lib\
         $$PWD/ffmpeg-4.2.2/lib/postproc.lib\
         $$PWD/ffmpeg-4.2.2/lib/swresample.lib\
         $$PWD/ffmpeg-4.2.2/lib/swscale.lib

include(./opengl/opengl.pri)
INCLUDEPATH+=./opengl/

#LIBS+= C:\Qt\opencv-release\lib\libopencv_*.dll.a
LIBS+= E:\colin\Qt\opencv\opencv-release\lib\libopencv_*.dll.a
DISTFILES +=
