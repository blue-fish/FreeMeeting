#ifndef SDLAUDIOWRITE_H
#define SDLAUDIOWRITE_H

#include <QObject>
#include"AudioWorld.h"
#include<mutex>
#include<deque>
class SDLAudioWrite : public QObject
{
    Q_OBJECT

signals:

public:
    explicit SDLAudioWrite(QObject *parent = nullptr);
    ~SDLAudioWrite();
public:
    //开始播放
    void slot_openAudio()
    {
        // 开始播放音频
        if( m_isOpen ) return;
        SDL_PauseAudioDevice(dev, 0);
        m_isOpen = true;
    }
   //关闭播放
    void slot_closeAudio()
    {
        // 停止播放音频
        if( !m_isOpen ) return;
        SDL_PauseAudioDevice(dev, 1);
        m_isOpen = false;
    }


public slots:
    void slot_playAudioFrame(QByteArray recvBuffer);
private:
     static void audioCallback(void *userdata, Uint8 *stream, int len);
private:
    SDL_AudioDeviceID dev;
    bool m_isOpen=false;
    std::deque<QByteArray> m_audioQueue;
    std::mutex m_mutex;
    OpusDecoder* decoder;
};

#endif // SDLAUDIOWRITE_H
