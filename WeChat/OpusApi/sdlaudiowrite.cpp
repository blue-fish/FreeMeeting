#include "sdlaudiowrite.h"

SDLAudioWrite::SDLAudioWrite(QObject *parent) : QObject(parent)
{
    // 初始化解码器
    int err;
    decoder = opus_decoder_create(48000, 1, &err);
    if (err < 0) {
        qDebug() << "创建Opus解码器失败:" << opus_strerror(err);
    }

    // 设置音频
    SDL_AudioSpec wantedSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_zero(wantedSpec);
    wantedSpec.freq = 48000;
    wantedSpec.format = AUDIO_S16LSB;
    wantedSpec.channels = 1;
    wantedSpec.samples = 960;
    wantedSpec.callback = audioCallback;
    wantedSpec.userdata = this;

    // 打开播放设备
    dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &obtainedSpec, 0);
    if (dev == 0) {
        qDebug() << "打开音频设备失败: " << SDL_GetError();
        SDL_Quit();
        return;
    }

    // 默认播放
    slot_openAudio();
}

SDLAudioWrite::~SDLAudioWrite()
{
    SDL_PauseAudioDevice(dev, 1);
    SDL_CloseAudioDevice(dev);

    if (decoder) {
        opus_decoder_destroy(decoder);
    }
}

void SDLAudioWrite::slot_playAudioFrame(QByteArray recvBuffer)
{
    if (!m_isOpen) return;

    // 先解码
    opus_int16 decodedData[960];  // 一帧的PCM数据
    int frameSizeDecoded = opus_decode(
        decoder,
        (const unsigned char*)recvBuffer.data(),
        recvBuffer.size(),
        decodedData,
        960,  // 最大帧大小
        0     // 不使用FEC
    );

    if (frameSizeDecoded < 0) {
        qDebug() << "Opus解码错误:" << opus_strerror(frameSizeDecoded);
        return;
    }

    // 将解码后的PCM数据发送给语音识别
    QByteArray pcmData((char*)decodedData, frameSizeDecoded * sizeof(opus_int16));
    emit SIG_sendDecodedPCM(pcmData);

    // 将数据加入播放队列
    std::lock_guard<std::mutex> lck(m_mutex);
    m_audioQueue.emplace_back(pcmData);
}

void SDLAudioWrite::audioCallback(void *userdata, Uint8 *stream, int len)
{
    SDLAudioWrite * audio = (SDLAudioWrite *)userdata;
    memset(stream, 0, len);

    if (!audio->m_audioQueue.empty())
    {
        QByteArray pcmData;
        {
            std::lock_guard<std::mutex> lck(audio->m_mutex);
            pcmData = audio->m_audioQueue.front();
            audio->m_audioQueue.pop_front();
        }

        // 混音播放
        SDL_MixAudioFormat(stream, (uint8_t*)pcmData.data(), AUDIO_S16LSB,
                          pcmData.size(), SDL_MIX_MAXVOLUME);
    }
}
