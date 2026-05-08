#ifndef AUDIOWORLD_H
#define AUDIOWORLD_H


#include<QDebug>
#include<QByteArray>
#include<QThread>
extern "C"{
#include <SDL.h>
#include<opus/opus.h>
}

#define USE_VAD  (1)

#include"WebRtc_Vad/webrtc_vad.h"

#endif // AUDIOWORLD_H
