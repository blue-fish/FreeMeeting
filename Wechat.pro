QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

RESOURCES += resourcce.qrc

include(./netapi/netapi.pri)
INCLUDEPATH += ./netapi

include(./MD5/md5.pri)
INCLUDEPATH += ./MD5

include(./audioapi/audioapi.pri)
INCLUDEPATH += ./audioapi/

include(./OpusApi/OpusApi.pri)
INCLUDEPATH += ./OpusApi/

include(./videoapi/videoapi.pri)
INCLUDEPATH += ./videoapi/

include(./sherpaApi/sherpaApi.pri)
INCLUDEPATH += ./sherpaApi/

INCLUDEPATH += $$PWD/ffmpeg-4.2.2/include
LIBS += $$PWD/ffmpeg-4.2.2/lib/avcodec.lib\
 $$PWD/ffmpeg-4.2.2/lib/avdevice.lib\
 $$PWD/ffmpeg-4.2.2/lib/avfilter.lib\
 $$PWD/ffmpeg-4.2.2/lib/avformat.lib\
 $$PWD/ffmpeg-4.2.2/lib/avutil.lib\
 $$PWD/ffmpeg-4.2.2/lib/postproc.lib\
 $$PWD/ffmpeg-4.2.2/lib/swresample.lib\
 $$PWD/ffmpeg-4.2.2/lib/swscale.lib

LIBS += -lWs2_32

FACE_DETECTION_DLL = $$PWD/videoapi/libfacedetection/bin/libfacedetection.dll
CONFIG(debug, debug|release) {
    FACE_DETECTION_OUT_DIR = $$OUT_PWD/debug
} else {
    FACE_DETECTION_OUT_DIR = $$OUT_PWD/release
}
exists($$FACE_DETECTION_DLL) {
    QMAKE_POST_LINK += $$quote($$QMAKE_COPY $$shell_path($$FACE_DETECTION_DLL) $$shell_path($$FACE_DETECTION_OUT_DIR/libfacedetection.dll))
}

HEADERS += \
    asrclient.h \
    avsyncmanager.h \
    ckernel.h \
    logindialog.h \
    roomdialog.h \
    usershow.h \
    videodecoder.h \
    videoencoder.h \
    wechatdialog.h

FORMS += \
    logindialog.ui \
    roomdialog.ui \
    usershow.ui \
    wechatdialog.ui

SOURCES += \
    asrclient.cpp \
    avsyncmanager.cpp \
    ckernel.cpp \
    logindialog.cpp \
    main.cpp \
    roomdialog.cpp \
    usershow.cpp \
    videodecoder.cpp \
    videoencoder.cpp \
    wechatdialog.cpp
