#ifndef SDLAUDIOWRITE_H
#define SDLAUDIOWRITE_H

#include <QObject>
#include "AudioWorld.h"
#include <mutex>
#include <deque>

class SDLAudioWrite : public QObject
{
    Q_OBJECT

signals:
    // 新增：发送解码后的PCM数据用于语音识别
    void SIG_sendDecodedPCM(QByteArray pcmData);

public:
    explicit SDLAudioWrite(QObject *parent = nullptr);
    ~SDLAudioWrite();

    void slot_openAudio() {
        if (m_isOpen) return;
        SDL_PauseAudioDevice(dev, 0);
        m_isOpen = true;
    }

    void slot_closeAudio() {
        if (!m_isOpen) return;
        SDL_PauseAudioDevice(dev, 1);
        m_isOpen = false;
    }

public slots:
    void slot_playAudioFrame(QByteArray recvBuffer);

private:
    static void audioCallback(void *userdata, Uint8 *stream, int len);

private:
    SDL_AudioDeviceID dev;
    bool m_isOpen = false;
    std::deque<QByteArray> m_audioQueue;
    std::mutex m_mutex;
    OpusDecoder* decoder;
};

#endif // SDLAUDIOWRITE_H
