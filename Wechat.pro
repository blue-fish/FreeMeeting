QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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
SOURCES += \
    avsyncmanager.cpp \
    ckernel.cpp \
    logindialog.cpp \
    main.cpp \
    roomdialog.cpp \
    usershow.cpp \
    videodecoder.cpp \
    videoencoder.cpp \
    wechatdialog.cpp

HEADERS += \
    avsyncmanager.h \
    cjson.h \
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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resourcce.qrc
