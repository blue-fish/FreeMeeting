QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    capturesample.cpp \
    facedetect.cpp \
    main.cpp \
    dialog.cpp

HEADERS += \
    capturesample.h \
    common.h \
    dialog.h \
    facedetect.h

FORMS += \
    dialog.ui

#三步 ： 添加头文件  添加动态库引入库 添加动态库
INCLUDEPATH += $$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/include\
            $$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/include/opencv2

LIBS+=$$PWD/opencv-4.5.4-with_contrib_qt_5.12.11_x86_mingw32/lib/libopencv_*.dll.a


INCLUDEPATH += $$PWD/libfacedetection/include

LIBS+=$$PWD/libfacedetection/lib/libfacedetection.dll.a

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
