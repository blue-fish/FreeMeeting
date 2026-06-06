#ifndef SDLAUDIOREAD_H
#define SDLAUDIOREAD_H

#include <QObject>
#include"AudioWorld.h"
class SDLAudioRead : public QObject
{
    Q_OBJECT

signals:
    void SIG_sendAudioFrame(QByteArray bt, int64_t timestamp);//多线程发送副本，所以没引用&bt
public:
    explicit SDLAudioRead(QObject *parent = nullptr);
    ~SDLAudioRead();
    void setUserId(int userId) { m_userId = userId; }
    //开启采集
    void slot_openAudio(){
        // 开始采集音频
        if( m_isOpen ) return;
        SDL_PauseAudioDevice(dev, 0);
        m_isOpen = true;
    }
    //关闭采集
    void slot_closeAudio(){
        // 停止采集音频
        if( !m_isOpen ) return;
        SDL_PauseAudioDevice(dev, 1);
        m_isOpen = false;
    }

private slots:
    void slot_sendAudioFrame(QByteArray &bt, int64_t timestamp);
private:
    static void audioCallback(void *userdata, Uint8 *stream, int len );

private:
    bool m_isOpen = false;
    //采集设备
    SDL_AudioDeviceID dev;
    OpusEncoder* encoder;
    int m_userId = 0;
    //webrtc vad 静音检测
     VadInst *handle;

};

#endif // SDLAUDIOREAD_H
