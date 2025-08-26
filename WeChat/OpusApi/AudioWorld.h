#ifndef AUDIOWORLD_H
#define AUDIOWORLD_H

#include <QDebug>
#include <QByteArray>

extern "C"{
#include <SDL.h>
#include <opus/opus.h>
}

#endif // AUDIOWORLD_H


//webrtc vad 静音检测
#define USE_VAD  (1)
#include"WebRtc_Vad/webrtc_vad.h"
