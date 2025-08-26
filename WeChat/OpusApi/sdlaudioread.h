#ifndef SDLAUDIOREAD_H
#define SDLAUDIOREAD_H

#include <QObject>

#include"AudioWorld.h"

class SDLAudioRead : public QObject
{
    Q_OBJECT

signals:
    void SIG_sendAudioFrame(QByteArray bt);//多线程发送副本
//signals:
    //void SIG_sendPCMData(QByteArray pcmData); // 新增：发送PCM数据用于识别

public:
    explicit SDLAudioRead(QObject *parent = nullptr);

    ~SDLAudioRead();

    // 开启采集
    void slot_openAudio(){
        // 开始采集音频
        if( m_isOpen ) return;
        SDL_PauseAudioDevice(dev, 0);
        m_isOpen = true;
    }
    // 关闭采集
    void slot_closeAudio(){
        // 停止采集音频
        if( !m_isOpen ) return;
        SDL_PauseAudioDevice(dev, 1);
        m_isOpen = false;
    }
private:
    static void audioCallback(void *userdata, Uint8 *stream, int len);

    void slot_sendAudioFrame(QByteArray &bt);
private:
    bool m_isOpen=false;

    //采集设备
    SDL_AudioDeviceID dev;
    //定义编码器
    OpusEncoder* encoder;

    //webrtc vad 静音检测
    VadInst* handle;

};

#endif // SDLAUDIOREAD_H
