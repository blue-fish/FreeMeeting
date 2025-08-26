#include "sdlaudioread.h"

SDLAudioRead::SDLAudioRead(QObject *parent) : QObject(parent)
{
    // 初始化 Opus 编码器
    int err;
    encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &err);
    if (err < 0) {
        qDebug() << "创建Opus编码器失败:" << opus_strerror(err);
    }

    // SDL 设置
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec obtainedSpec;
    SDL_memset(&desiredSpec, 0, sizeof(desiredSpec));
    desiredSpec.freq = 48000;        // 采样率 48000Hz
    desiredSpec.format = AUDIO_S16LSB; // 16位小端存储
    desiredSpec.channels = 1;         // 单声道
    desiredSpec.samples = 960;        // 20ms的样本数 (48000*0.02=960)
    desiredSpec.callback = audioCallback;
    desiredSpec.userdata = this;

    // 打开录音设备
    dev = SDL_OpenAudioDevice(NULL, 1, &desiredSpec, &obtainedSpec, 0);
    if (dev == 0) {
        qDebug() << "打开音频设备失败: " << SDL_GetError();
        SDL_Quit();
        return;
    }

    // 创建VAD
    if (0 != WebRtcVad_Create(&handle)) {
        qDebug() << "VAD创建失败";
    }
    // 初始化VAD
    if (0 != WebRtcVad_Init(handle)) {
        qDebug() << "VAD初始化失败";
    }
    // 设置VAD的灵敏度 (0-3, 3最灵敏)
    WebRtcVad_set_mode(handle, 3);
}

SDLAudioRead::~SDLAudioRead()
{
    SDL_PauseAudioDevice(dev, 1);
    SDL_CloseAudioDevice(dev);

    if (encoder) {
        opus_encoder_destroy(encoder);
    }

    if (handle) {
        WebRtcVad_Free(handle);
    }
}

void SDLAudioRead::slot_sendAudioFrame(QByteArray & bt)
{
    Q_EMIT SIG_sendAudioFrame(bt);
}

// 这是核心修改部分！
void SDLAudioRead::audioCallback(void *userdata, Uint8 *stream, int len)
{
    SDLAudioRead * audio = (SDLAudioRead *)userdata;

    // 检查数据长度是否正确 (960样本 * 2字节 = 1920字节)
    if (len != 1920) {
        qWarning() << "音频回调: 数据长度异常" << len << ", 期望1920";
        return;
    }

    // ========== 第一步：静音检测 ==========
    int vad_result = -1;
    if (audio->handle) {
        // VAD处理：检测是否有语音
        vad_result = WebRtcVad_Process(
            audio->handle,           // VAD句柄
            48000,                   // 采样率
            (int16_t*)stream,        // 音频数据
            960                      // 样本数（不是字节数！）
        );

        if (vad_result == -1) {
            qWarning() << "VAD处理错误";
            return;
        }
    }

    // ========== 第二步：根据VAD结果处理 ==========
    if (vad_result == 1) {  // 检测到语音（非静音）
        //qDebug() << "检测到语音活动";

        // 对非静音进行Opus编码
        unsigned char opusData[4096];
        int nbBytes = opus_encode(
            audio->encoder,           // 编码器
            (const opus_int16*)stream, // PCM数据
            960,                      // 每帧样本数
            opusData,                 // 输出缓冲区
            sizeof(opusData)          // 缓冲区大小
        );

        if (nbBytes < 0) {
            qDebug() << "Opus编码错误:" << opus_strerror(nbBytes);
            return;
        }

        // 发送编码后的数据
        QByteArray sendBuffer((char*)opusData, nbBytes);
        audio->slot_sendAudioFrame(sendBuffer);

    } else if (vad_result == 0) {  // 静音
        //qDebug() << "检测到静音";
        // 静音时不发送数据，节省带宽
        // 如果需要，可以发送一个特殊的静音帧标记
    }
}
