QT+=multimedia


#SDL 的添加
INCLUDEPATH += $$PWD/SDL2-2.0.10/include\
               $$PWD/opus-1.4/include\
               $$PWD/WebRtc_Vad\
               $$PWD/WebRtcMoudle


LIBS += $$PWD/SDL2-2.0.10/lib/x86/SDL2.lib\
        $$PWD/opus-1.4/lib/libopus.a -lssp\
        $$PWD/WebRtc_Vad/signalProcess.lib

HEADERS += \
    $$PWD/AudioWorld.h \
    $$PWD/sdlaudioread.h \
    $$PWD/sdlaudiowrite.h

SOURCES += \
    $$PWD/sdlaudioread.cpp \
    $$PWD/sdlaudiowrite.cpp
