#ifndef AUDIOREAD_H
#define AUDIOREAD_H

#include <QObject>
#include"word.h"

class AudioRead : public QObject
{
    Q_OBJECT
public:
    explicit AudioRead(QObject *parent = nullptr);
    ~AudioRead();

signals:
    void SIG_audioFrame(QByteArray ba);
private:
    QAudioInput* m_audio_in; // 采集声卡 音频输入
    QIODevice* m_buffer_in; // 对应的缓冲区
    QTimer* m_timer;
    int m_audioState ;
    QAudioFormat format;
    //speex相关变量
    SpeexBits bits_enc;
     void *Enc_State;

     //webrtc vad 静音检测
     VadInst * handle;

     //降噪

public slots:
    void ReadMore();//定时超时，从缓冲区读取数据
    void pause();
    void start();
};

#endif // AUDIOREAD_H
